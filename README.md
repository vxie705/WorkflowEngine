# ⚙️ Workflow Engine

Un framework de C++20 para orquestar pipelines de procesamiento de datos mediante una arquitectura **Pipe & Filter** modular y desacoplada.

![C++20](https://img.shields.io/badge/C%2B%2B-20-blue?logo=c%2B%2B) 
![CMake](https://img.shields.io/badge/CMake-3.20%2B-green?logo=cmake) 
![License](https://img.shields.io/badge/license-MIT-yellow)
![Tests](https://img.shields.io/badge/tests-41%2F41%20passed-brightgreen)

## 📖 Resumen
**Workflow Engine** permite definir lógica de negocio compleja como una secuencia de pasos independientes (**Commands**) configurados vía **JSON**.

* **Modular:** Cada paso es una unidad independiente e inyectable.
* **Resiliente:** Manejo de errores basado en monadas (`Result<T>`) sin excepciones ocultas.
* **Observabilidad:** Auditoría automática de estados con snapshots en cada paso.
* **Desacoplado:** Comunicación centralizada a través de un `DataBus`.

## 🏗️ Arquitectura



El motor parsea un archivo JSON para ejecutar una serie de comandos de forma ordenada, compartiendo estado a través del `DataBus` y registrando la actividad mediante `ILogger`.

## 🚀 Uso Rápido

**Requisitos:** C++20, CMake 3.20+.

```bash
# Compilar y construir
cmake -B build
cmake --build build --config Release

# Ejecutar el demo
./build/workflow_demo

{
    "pipeline": [
        { "type": "EchoCommand", "name": "start", "params": { "message": "Iniciando..." } },
        { "type": "TransformCommand", "name": "clean", "params": { "input_key": "raw", "transform": "uppercase" } }
    ]
}
