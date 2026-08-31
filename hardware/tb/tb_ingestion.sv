`timescale 1ns / 1ps

module tb_ingestion;

  // Parámetros de Simulación y Constantes Temporales
  localparam int CLK_PERIOD_NS = 10;  // Reloj de 100MHz
  localparam int CLKS_PER_BIT = 868;
  // Duración exacta de 1 bit serie en nanosegundos (868 * 10 ns = 8680 ns)
  localparam int BIT_PERIOD_NS = CLKS_PER_BIT * CLK_PERIOD_NS;

  // Señales de Interconexión del Banco de Pruebas
  logic        clk;
  logic        rst_n;
  logic        rx_serial;  // Línea serie virtual inyectada a la FPGA

  // Cables intermedios entre uart_rx y tick_parser_fsm
  logic [ 7:0] rx_byte;
  logic        rx_valid;

  // Salidas verificables de la FSM
  logic [63:0] price_out;
  logic [ 7:0] type_out;
  logic        data_valid;

  // Instanciación del DUT (Device Under Test)
  // Receptor UART
  uart_rx #(
      .CLKS_PER_BIT(CLKS_PER_BIT)
  ) u_uart_rx (
      .clk     (clk),
      .rst_n   (rst_n),
      .rx      (rx_serial),
      .rx_data (rx_byte),
      .rx_valid(rx_valid)
  );

  // Parser Deserializador
  tick_parser_fsm u_parser (
      .clk       (clk),
      .rst_n     (rst_n),
      .rx_data   (rx_byte),
      .rx_valid  (rx_valid),
      .price_out (price_out),
      .type_out  (type_out),
      .data_valid(data_valid)
  );


  // Generador del Reloj Maestro de 100 MHz
  initial clk = 1'b0;
  always #(CLK_PERIOD_NS / 2) clk = ~clk;

  // Tarea de Emulación de Transmisión UART (Host PC -> FPGA)
  task automatic send_uart_byte(input logic [7:0] data_byte);
    int i;
    begin
      // 1. Bit de Start (Transición a nivel bajo)
      rx_serial = 1'b0;
      #(BIT_PERIOD_NS);

      // 2. 8 Bits de Datos (Enviados de LSB a MSB según estándar UART)
      for (i = 0; i < 8; i = i + 1) begin
        rx_serial = data_byte[i];
        #(BIT_PERIOD_NS);
      end

      // 3. Bit de Stop (Retorno a nivel alto de reposo)
      rx_serial = 1'b1;
      #(BIT_PERIOD_NS);
    end
  endtask

  // Tarea de Envío del Paquete Completo (10 Bytes)
  task automatic send_tick_packet(input logic [7:0] order_type, input logic [63:0] price_raw);
    int b;
    begin
      $display("[TB] Enviando paquete: Tipo = %c, Precio Raw = 0x%016h", order_type, price_raw);

      // Byte 0: Byte Mágico 'T' (0x54)
      send_uart_byte(8'h54);

      // Byte 1: Tipo de Orden ('B' o 'A')
      send_uart_byte(order_type);

      // Bytes 2 a 9: Precio en Little Endian (Byte menos significativo primero)
      for (b = 0; b < 8; b = b + 1) begin
        send_uart_byte(price_raw[(b*8)+:8]);
      end
    end
  endtask

  // Secuencia Principal de Verificación
  initial begin
    // Valor de prueba: 1.16492 -> 0x3FF2A39B5A9D28B2 en IEEE 754 doble precisión
    localparam logic [63:0] EXPECTED_PRICE = 64'h3FF2A39B5A9D28B2;
    localparam logic [7:0] EXPECTED_TYPE = 8'h42;  // 'B'

    // A. Inicialización y Reset
    rx_serial = 1'b1;  // Línea UART en reposo es siempre nivel alto (1)
    rst_n     = 1'b0;  // Aplicar reset activo a nivel bajo
    #(100);  // Mantener reset durante 100 ns

    rst_n = 1'b1;  // Liberar reset
    #(200);

    // B. Inyección del paquete por la línea serie virtual
    $display("\n========================================================");
    $display("[TB] INICIANDO PRUEBA DE INGESTA UART Y PARSER");
    $display("========================================================");

    send_tick_packet(EXPECTED_TYPE, EXPECTED_PRICE);

    // C. Espera de la bandera data_valid emitida por la FSM
    // Esperamos un máximo de 100 microsegundos antes de declarar Timeout
    fork : wait_strobe
      begin
        @(posedge data_valid);
        $display("[TB] Pulso data_valid detectado en t = %0t ps", $time);
        disable wait_strobe;
      end
      begin
        #(150_000);  // 150 us de tiempo límite
        $fatal(1, "[-] ERROR CRÍTICO: Timeout esperando pulso data_valid.");
      end
    join

    // D. Verificación de los datos deserializados en el mismo ciclo
    #(CLK_PERIOD_NS);  // Esperar 1 ciclo de clk para comprobar estabilidad

    if (price_out === EXPECTED_PRICE && type_out === EXPECTED_TYPE) begin
      $display("\n[+] VERIFICACIÓN EXITOSA:");
      $display("    - Tipo Esperado:   %c  | Obtenido: %c", EXPECTED_TYPE, type_out);
      $display("    - Precio Esperado: 0x%016h | Obtenido: 0x%016h", EXPECTED_PRICE, price_out);
      $display("========================================================\n");
    end else begin
      $error("[-] FALLO EN LA VALIDACIÓN:");
      $display("    - Tipo Esperado:   %c  | Obtenido: %c", EXPECTED_TYPE, type_out);
      $display("    - Precio Esperado: 0x%016h | Obtenido: 0x%016h", EXPECTED_PRICE, price_out);
      $fatal(1, "Los datos reconstruidos no coinciden.");
    end

    // Pausa adicional y cierre
    #(1000);
    $finish;
  end

endmodule
