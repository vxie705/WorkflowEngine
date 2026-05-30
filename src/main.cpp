#include "WorkflowEngine.hpp"
#include "ILogger.hpp"
#include "ConsoleLogger.hpp"


#include "EchoCommand.hpp"
#include "DelayCommand.hpp"
#include "TransformCommand.hpp"

#include <iostream>
#include <memory>

int main() {

    auto logger = std::make_unique<workflow::ConsoleLogger>();
    auto bus = std::make_unique<workflow::DataBus>();

    workflow::WorkflowEngine engine(std::move(logger), std::move(bus));




    engine.sync_from_registry();





    workflow::DataPacket initial_data;
    initial_data.set<std::string>("user", "juan_perez");


    std::cout << "\n========================================" << std::endl;
    std::cout << "  WORKFLOW ENGINE — DEMO DE EJECUCION" << std::endl;
    std::cout << "  (Auto-registro via CommandRegistry)" << std::endl;
    std::cout << "========================================\n" << std::endl;

    auto result = engine.execute_from_file("config/workflow.json", std::move(initial_data));

    if (result.is_ok()) {
        std::cout << "\nPipeline completado exitosamente." << std::endl;
        std::cout << "Entradas en el DataPacket final: "
                  << result.value().size() << std::endl;
        return 0;
    } else {
        std::cout << "\nPipeline FALLIDO:" << std::endl;
        std::cout << "  Error: " << result.error_message() << std::endl;
        std::cout << "  Codigo: " << result.error_code() << std::endl;
        return 1;
    }
}