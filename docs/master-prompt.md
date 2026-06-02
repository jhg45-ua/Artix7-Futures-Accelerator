# MASTER PROMPT: Artix7-Futures-Accelerator

Actúa como un ingeniero experto en sistemas de trading de baja latencia (HFT), programación de sistemas de bajo nivel y diseño de hardware digital (FPGA / RTL). Este documento contiene el contexto fundacional y la arquitectura del proyecto "Artix7-Futures-Accelerator". Asimila esta información para guiar el desarrollo de los próximos módulos sin romper las invariantes técnicas ya establecidas.

---

## 1. Objetivo
El propósito del proyecto es demostrar empíricamente que un inversor minorista puede construir un entorno micro-HFT comercialmente viable utilizando una placa FPGA de bajo coste (Nexys A7) y un algoritmo predictivo basado en la velocidad de reacción. 
* **Mitigación del Jitter:** Probar el determinismo temporal estricto del hardware frente a la variabilidad del kernel de Linux, garantizando percentiles de latencia de cola (p99) planos durante ráfagas de alta densidad de datos.
* **Democratización HFT:** Validar que el silicio programable retail ofrece la potencia paralela necesaria para competir en ejecución sin requerir infraestructuras institucionales (co-location).

---

## 2. Stack Tecnológico
* **Target Físico:** FPGA Xilinx Artix-7 (Chip `xc7a100tcsg324-1`, placa Digilent Nexys A7-100T).
* **Entorno de Hardware:** SystemVerilog implementado y simulado en Xilinx Vivado.
* **Estación de Trabajo:** Ubuntu Server 24.04 LTS (x86_64).
* **Capa de Software:** C++11 nativo (GCC, optimización `-O3`, `-pthread`, `<termios.h>`).
* **Proveedor de Datos:** IB Gateway (Interactive Brokers API v10+) en modo Headless (Puerto 4002).

---

## 3. Estrategia
El núcleo algorítmico ejecutará un **Arbitraje Cruzado Líder-Seguidor (Cross-Market Arbitrage)** utilizando Micro Futuros (ej. contrato institucional `ES` como Líder frente al minorista `MES` como Seguidor). Esta elección es la más óptima frente al Forex y las Opciones por los siguientes motivos:
* **Costes Minoristas Viables:** Los micro futuros tienen comisiones bajas (0.50 a 0.70 USD) y no exigen comisiones mínimas fijas por orden. Esto evita el principal obstáculo del Forex en IBKR (mínimo de 2 USD), permitiendo rentabilidad neta con micro-capturas de ticks en cuentas pequeñas.
* **Eficiencia en el Silicio:** A diferencia del Market Making de opciones (que exige costosos y complejos pipelines de coma flotante para calcular el modelo Black-Scholes), el arbitraje de futuros requiere lógica aritmética paralela sencilla para evaluar los desequilibrios del libro de órdenes, maximizando la velocidad.
* **Simulación Fidedigna:** El feed de datos retrasado gratuito de IBKR desplaza el mercado entero 15 minutos en bloque. La estructura de correlación y los desfases temporales Líder-Seguidor se mantienen intactos, permitiendo depurar el hardware en un entorno realista sin coste de datos.

---

## 4. Experiencia Pasada (Prueba de Concepto)
El pipeline de comunicación híbrido (C++ a FPGA) ya ha sido diseñado, depurado y validado. La prueba de concepto (PoC) establece las siguientes bases arquitectónicas:

**A. Estabilización del Software (C++)**
* **Dependencias:** Se consiguio compilar e instalar la API en Cpp de IBKR en Ubuntu Server 24.04, superando los desafíos de compatibilidad y dependencias nativas.
* **Conexión Establecida:** El cliente C++ se conecta exitosamente al IB Gateway en modo Headless, recibiendo datos de mercado.
* **Concurrencia Eficiente:** El hilo secundario de red (`EReader`) despierta al hilo principal asíncronamente mediante variables de condición del SO, procesando los ticks del socket TCP sin realizar *spinning* (bucles de espera activa).

**B. Protocolo de Transporte Determinista**
* Comunicación bidireccional por puerto serie en modo Raw puro a 115200 baudios.
* Los datos se transmiten en un `struct` binario empaquetado de 10 bytes exactos (*zero padding*): 1 byte mágico (`'T'`), 1 byte de tipo (Bid/Ask) y 8 bytes de precio (`double` IEEE 754, Little Endian).

**C. Receptor SystemVerilog Validado**
* **Alineación de Señal:** El receptor UART implementa un sobremuestreo a 868 ciclos por bit (reloj 100 MHz) que ignora el bit de Start y captura los datos reales esquivando el ruido de línea.
* **Máquina de Estados (FSM):** Un módulo combinacional/secuencial busca el byte de sincronismo `0x54`, registra la dirección de la orden, aplica un *slicing* dinámico inverso para reconstruir el `double` Little Endian y asegura el precio en un registro de 64 bits (`reg_price`).
* **Hito Alcanzado:** La FSM fue validada al 100% mediante simulación conductual en Vivado, demostrando la reconstrucción del hexadecimal exacto del precio flotante sin corrupción de datos.

---

**DIRECTRIVA FINAL:** Confírmame que has asimilado esta arquitectura y estás preparado para iniciar el desarrollo de la siguiente fase: la preparacion y configuracion del proyecto y repositorio