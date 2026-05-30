#include "WorkflowEngine.hpp"
#include "CommandRegistry.hpp"
#include "WorkflowSchema.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

#include <nlohmann/json.hpp>

namespace workflow {

namespace fs = std::filesystem;

namespace {

std::string utc_timestamp() {
  auto now = std::chrono::system_clock::now();
  auto tt = std::chrono::system_clock::to_time_t(now);
  std::tm tm_buf{};
#ifdef _WIN32
  gmtime_s(&tm_buf, &tt);
#else
  gmtime_r(&tt, &tm_buf);
#endif
  char buf[64];
  std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_buf);
  return std::string(buf);
}

std::string sanitize_filename(const std::string &name) {
  std::string safe;
  safe.reserve(name.size());
  for (char c : name) {
    if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_') {
      safe.push_back(c);
    } else {
      safe.push_back('_');
    }
  }
  return safe;
}
} // namespace

WorkflowEngine::WorkflowEngine(std::unique_ptr<ILogger> logger,
                               std::unique_ptr<DataBus> bus)
    : logger_(std::move(logger)), bus_(std::move(bus)) {
  if (!bus_) {
    bus_ = std::make_unique<DataBus>();
  }
}

WorkflowEngine::~WorkflowEngine() = default;

WorkflowEngine::WorkflowEngine(WorkflowEngine &&) noexcept = default;
WorkflowEngine &WorkflowEngine::operator=(WorkflowEngine &&) noexcept = default;

void WorkflowEngine::register_command_factory(const std::string &type_name,
                                              CommandFactory factory) {
  registry_[type_name] = std::move(factory);
  if (logger_) {
    logger_->debug("Registered command type: " + type_name);
  }
}

void WorkflowEngine::sync_from_registry() {
  auto &global = CommandRegistry::instance();
  logger_->info("Syncing from global CommandRegistry (" +
                std::to_string(global.size()) + " types available)");
  global.export_to(registry_);

  if (logger_) {
    auto names = global.type_names();
    for (const auto &name : names) {
      logger_->debug("  - " + name);
    }
  }
}

size_t WorkflowEngine::load_plugins(const std::string &plugin_dir) {
  if (logger_) {
    logger_->info("Loading plugins from directory: " + plugin_dir);
  }

  auto loaded = plugin_loader_.load_from_directory(plugin_dir);

  if (logger_) {
    logger_->info("Loaded " + std::to_string(loaded.size()) + " plugins");
    for (const auto &plugin : loaded) {
      logger_->info("  - " + plugin.name + " (" + plugin.path + ")");
    }
  }

  return loaded.size();
}

size_t WorkflowEngine::load_plugins_and_sync(const std::string &plugin_dir) {
  size_t count = load_plugins(plugin_dir);
  sync_from_registry();
  return count;
}

void WorkflowEngine::write_audit_snapshot(const std::string &workflow_name,
                                          size_t step_index,
                                          const std::string &command_name,
                                          const std::string &status,
                                          const nlohmann::json &snapshot) {
  if (audit_dir_.empty()) {
    audit_dir_ = "logs/audit";
  }

  std::error_code ec;
  fs::create_directories(audit_dir_, ec);
  if (ec) {
    logger_->warn("Audit: cannot create directory '" + audit_dir_ +
                  "': " + ec.message());
    return;
  }

  std::string safe_name = sanitize_filename(workflow_name);
  std::string filename = safe_name + "_step" + std::to_string(step_index + 1) +
                         "_" + sanitize_filename(command_name) + "_" + status +
                         ".json";

  fs::path filepath = fs::path(audit_dir_) / filename;

  nlohmann::json record;
  record["workflow"] = workflow_name;
  record["step"] = step_index + 1;
  record["command"] = command_name;
  record["status"] = status;
  record["timestamp"] = utc_timestamp();
  record["snapshot"] = snapshot;

  fs::path tmp_path = fs::path(audit_dir_) / ("." + filename + ".tmp");
  {
    std::ofstream out(tmp_path);
    if (!out) {
      logger_->warn("Audit: cannot open file '" + tmp_path.string() +
                    "' for writing");
      return;
    }
    out << record.dump(2 /* indent */);
    out.close();
  }

  fs::rename(tmp_path, filepath, ec);
  if (ec) {
    logger_->warn("Audit: cannot rename '" + tmp_path.string() + "' to '" +
                  filepath.string() + "': " + ec.message());
    return;
  }

  logger_->debug("Audit snapshot written: " + filepath.string());
}

nlohmann::json WorkflowEngine::build_snapshot(const DataPacket &data_packet,
                                              const DataBus &bus) {
  nlohmann::json snap;
  snap["data_packet"] = data_packet.to_json();
  snap["data_bus"] = bus.to_json();
  snap["timestamp"] = utc_timestamp();
  return snap;
}

Result<DataPacket>
WorkflowEngine::execute_from_file(const std::string &config_path) {
  logger_->info("Loading workflow from: " + config_path);

  auto config_result = WorkflowConfig::load_from_file(config_path);
  if (config_result.is_error()) {
    logger_->error("Failed to load config: " + config_result.error_message());
    return Result<DataPacket>::error(config_result.error_message(),
                                     config_result.error_code());
  }

  return execute(config_result.value());
}

Result<DataPacket>
WorkflowEngine::execute_from_file(const std::string &config_path,
                                  DataPacket initial) {
  logger_->info("Loading workflow from: " + config_path);

  auto config_result = WorkflowConfig::load_from_file(config_path);
  if (config_result.is_error()) {
    logger_->error("Failed to load config: " + config_result.error_message());
    return Result<DataPacket>::error(config_result.error_message(),
                                     config_result.error_code());
  }

  return execute(config_result.value(), std::move(initial));
}

Result<DataPacket> WorkflowEngine::execute(const WorkflowDefinition &config) {
  logger_->info("Executing workflow: " + config.name);
  logger_->debug("Pipeline has " + std::to_string(config.pipeline.size()) +
                 " steps");

  bus_->clear();

  DataPacket initial_input;
  return execute_pipeline(config, std::move(initial_input));
}

Result<DataPacket> WorkflowEngine::execute(const WorkflowDefinition &config,
                                           DataPacket initial) {
  logger_->info("Executing workflow: " + config.name);
  logger_->debug("Pipeline has " + std::to_string(config.pipeline.size()) +
                 " steps");

  bus_->clear();

  return execute_pipeline(config, std::move(initial));
}

Result<DataPacket>
WorkflowEngine::execute_pipeline(const WorkflowDefinition &config,
                                 DataPacket initial_input) {
  DataPacket current = std::move(initial_input);
  const auto &pipeline = config.pipeline;
  const bool audit_enabled = config.audit;
  const OnError on_error = config.on_error;

  for (size_t i = 0; i < pipeline.size(); ++i) {
    const auto &cfg = pipeline[i];

    auto it = registry_.find(cfg.type);
    if (it == registry_.end()) {
      std::string msg = "Unknown command type '" + cfg.type +
                        "' at pipeline step " + std::to_string(i) + " (" +
                        cfg.instance_name + ")";
      logger_->error(msg);
      return Result<DataPacket>::error(msg);
    }

    std::unique_ptr<ICommand> cmd;
    try {
      cmd = it->second(cfg.instance_name, cfg.params);
    } catch (const std::exception &e) {
      std::string msg = "Failed to instantiate command '" + cfg.instance_name +
                        "': " + e.what();
      logger_->error(msg);
      return Result<DataPacket>::error(msg);
    }

    if (!cmd) {
      std::string msg =
          "Factory returned nullptr for '" + cfg.instance_name + "'";
      logger_->error(msg);
      return Result<DataPacket>::error(msg);
    }

    for (const auto &dep : cfg.depends_on) {
      if (!bus_->has_key(dep)) {
        logger_->warn("Command '" + cfg.instance_name +
                      "' depends on bus key '" + dep +
                      "' which is not present");
      }
    }

    logger_->info("[" + std::to_string(i + 1) + "/" +
                  std::to_string(pipeline.size()) +
                  "] Executing: " + cfg.instance_name + " (" + cfg.type + ")");

    nlohmann::json pre_snapshot;
    if (audit_enabled) {
      pre_snapshot = build_snapshot(current, *bus_);
      pre_snapshot["phase"] = "pre-execution";
      write_audit_snapshot(config.name, i, cfg.instance_name, "pre",
                           pre_snapshot);
    }

    std::optional<Result<DataPacket>> maybe_result;
    bool exception_occurred = false;
    std::string exception_msg;

    try {
      maybe_result = cmd->execute(current, *bus_, *logger_);
    } catch (const std::exception &e) {
      exception_occurred = true;
      exception_msg = std::string("Command '") + cfg.instance_name +
                      "' threw exception: " + e.what();
      logger_->error(exception_msg);
    } catch (...) {
      exception_occurred = true;
      exception_msg = std::string("Command '") + cfg.instance_name +
                      "' threw unknown exception";
      logger_->error(exception_msg);
    }

    if (exception_occurred) {
      if (audit_enabled) {

        nlohmann::json failure_snapshot = build_snapshot(current, *bus_);
        failure_snapshot["phase"] = "pre-failure";
        failure_snapshot["error"] = exception_msg;

        write_audit_snapshot(config.name, i, cfg.instance_name, "exception",
                             failure_snapshot);
        logger_->info("Post-mortem snapshot saved for: " + cfg.instance_name);
      }

      if (on_error == OnError::HALT) {
        return Result<DataPacket>::error(exception_msg, -1);
      }

      logger_->warn(
          "on_error=continue: advancing to next step despite exception");
      if (audit_enabled) {

        nlohmann::json post_snap = build_snapshot(current, *bus_);
        post_snap["phase"] = "post-execution";
        post_snap["status"] = "skipped_due_to_exception";
        write_audit_snapshot(config.name, i, cfg.instance_name, "skipped",
                             post_snap);
      }
      continue;
    }

    if (maybe_result && maybe_result->is_error()) {
      logger_->error("Pipeline halted at step " + std::to_string(i + 1) + " '" +
                     cfg.instance_name + "': " + maybe_result->error_message());

      if (audit_enabled) {
        nlohmann::json error_snapshot = build_snapshot(current, *bus_);
        error_snapshot["phase"] = "pre-failure";
        error_snapshot["error"] = maybe_result->error_message();
        error_snapshot["error_code"] = maybe_result->error_code();

        write_audit_snapshot(config.name, i, cfg.instance_name, "error",
                             error_snapshot);
        logger_->info("Error snapshot saved for: " + cfg.instance_name);
      }

      if (on_error == OnError::HALT) {
        return std::move(*maybe_result);
      }

      logger_->warn("on_error=continue: advancing to next step despite error");
      if (audit_enabled) {
        nlohmann::json post_snap = build_snapshot(current, *bus_);
        post_snap["phase"] = "post-execution";
        post_snap["status"] = "skipped_due_to_error";
        write_audit_snapshot(config.name, i, cfg.instance_name, "skipped",
                             post_snap);
      }
      continue;
    }

    if (audit_enabled && maybe_result) {

      nlohmann::json post_snapshot =
          build_snapshot(maybe_result->value(), *bus_);
      post_snapshot["phase"] = "post-execution";
      write_audit_snapshot(config.name, i, cfg.instance_name, "success",
                           post_snapshot);
    }

    current = std::move(maybe_result->value());
    logger_->debug("Step " + std::to_string(i + 1) + " completed. " +
                   "DataPacket now has " + std::to_string(current.size()) +
                   " entries.");
  }

  logger_->info("Workflow completed successfully.");
  return Result<DataPacket>::ok(std::move(current));
}

} // namespace workflow