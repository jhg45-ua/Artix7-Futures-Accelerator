`timescale 1ns / 1ps

module tb_hft_pipeline;

  // Parámetros Temporales y Físicos
  localparam int CLK_PERIOD_NS  = 10;                            // 100 MHz (10 ns)
  localparam int CLKS_PER_BIT   = 868;                           // 100 MHz / 115200 baudios
  localparam int BIT_PERIOD_NS  = CLKS_PER_BIT * CLK_PERIOD_NS;  // 8680 ns (~8.68 µs)
  localparam int BYTE_PERIOD_NS = BIT_PERIOD_NS * 10;            // ~86.8 µs por byte

  // Señales de Interfaz con hft_top
  logic        clk_100mhz;
  logic        btn_rst_n;
  logic        uart_rx_in;
  logic        uart_tx_out;
  logic [15:0] led;

  // Instancia del Top-Level DUT
  hft_top #(
      .CLKS_PER_BIT(CLKS_PER_BIT)
  ) dut (
      .clk_100mhz (clk_100mhz),
      .btn_rst_n  (btn_rst_n),
      .uart_rx_in (uart_rx_in),
      .uart_tx_out(uart_tx_out),
      .led        (led)
  );

  // Generador de Reloj Maestro (100 MHz continuo)
  initial clk_100mhz = 1'b0;
  always #(CLK_PERIOD_NS / 2) clk_100mhz = ~clk_100mhz;

  // Tareas de Emulación de Capa Física UART

  // Envía 1 byte con protocolo 8N1 (Start=0, 8 bits LSB->MSB, Stop=1)
  task automatic send_uart_byte(input logic [7:0] data_byte);
    begin
      // 1. Start Bit
      uart_rx_in <= 1'b0;
      #(BIT_PERIOD_NS);

      // 2. Data Bits (LSB a MSB)
      for (int i = 0; i < 8; i++) begin
        uart_rx_in <= data_byte[i];
        #(BIT_PERIOD_NS);
      end

      // 3. Stop Bit
      uart_rx_in <= 1'b1;
      #(BIT_PERIOD_NS);
    end
  endtask

  // Empaqueta y transmite el FpgaTickPacket de 10 bytes (0x54 + Tipo + IEEE-754)
  task automatic send_tick_packet(input logic [7:0] side, input logic [63:0] price);
    begin
      // Byte 0: Magic Sync (0x54 = 'T')
      send_uart_byte(8'h54);

      // Byte 1: Tipo ('B' o 'A')
      send_uart_byte(side);

      // Bytes 2 a 9: Precio en Little Endian
      for (int i = 0; i < 8; i++) begin
        send_tick_packet_byte : send_uart_byte
        (price[i * 8 +: 8]);
      end
    end
  endtask

  // Captura 1 byte desde la línea de transmisión de la FPGA (uart_tx_out)
  task automatic receive_uart_byte(output logic [7:0] captured_byte);
    begin
      @(negedge uart_tx_out);  // Detección del Start Bit (1 -> 0)
      #(BIT_PERIOD_NS + (BIT_PERIOD_NS / 2));  // Muestreo en el centro de bit 0

      for (int i = 0; i < 8; i++) begin
        captured_byte[i] = uart_tx_out;
        #(BIT_PERIOD_NS);
      end

      #(BIT_PERIOD_NS / 2);  // Salida limpia al final del Stop Bit
    end
  endtask

  // Captura el paquete completo de ejecución FpgaOrderPacket (10 bytes)
  task automatic receive_order_packet(output logic [7:0] magic, output logic [7:0] action,
                                      output logic [31:0] order_id,
                                      output logic [31:0] hw_timestamp);
    logic [7:0] raw_bytes[0:9];
    begin
      for (int b = 0; b < 10; b++) begin
        receive_uart_byte(raw_bytes[b]);
        $display("    [UART RX Monitor] Byte [%0d/9]: 0x%02X (%c)", b, raw_bytes[b],
                 (raw_bytes[b] >= 32 && raw_bytes[b] <= 126) ? raw_bytes[b] : ".");
      end

      magic        = raw_bytes[0];
      action       = raw_bytes[1];
      order_id     = {raw_bytes[5], raw_bytes[4], raw_bytes[3], raw_bytes[2]};
      hw_timestamp = {raw_bytes[9], raw_bytes[8], raw_bytes[7], raw_bytes[6]};
    end
  endtask

  // Secuencia Principal de Simulación
  initial begin
    // Precios de prueba IEEE 754 (Double Precision)
    localparam logic [63:0] PRICE_ASK_INIT = 64'h40F2FD8000000000;  // 77790.0
    localparam logic [63:0] PRICE_BID_LOW = 64'h40F2FD8000000000;  // 77790.0
    localparam logic [63:0] PRICE_BID_HIGH = 64'h40F2FE6000000000;  // 77795.0 (> 77792.0 Umbral)

    logic [7:0] rx_magic, rx_action;
    logic [31:0] rx_order_id, rx_timestamp;
    bit order_received = 0;

    // Inicialización de líneas
    uart_rx_in <= 1'b1;
    btn_rst_n  <= 1'b0;
    #(200);
    btn_rst_n <= 1'b1;  // Liberar Reset
    #(500);

    $display("\n===================================================================");
    $display("[TB] INICIANDO SIMULACIÓN INTEGRAL DEL PIPELINE HFT (END-TO-END)");
    $display("===================================================================");

    // PASO 1: Inyectar Ask Inicial
    $display("[+] Enviando Tick 1: ASK Inicial = 77790.0...");
    send_tick_packet(8'h41, PRICE_ASK_INIT);  // 'A' = 0x41
    #(20_000);

    // PASO 2: Inyectar Bid Bajo
    $display("[+] Enviando Tick 2: BID Normal = 77790.0 (< 77792.0 Umbral)...");
    send_tick_packet(8'h42, PRICE_BID_LOW);  // 'B' = 0x42
    #(20_000);

    if (led[1] === 1'b1 && led[2] === 1'b0) begin
      $display("[+] Telemetría: Top of Book LISTO (LD1=1) y Disparo INACTIVO (LD2=0) [OK]");
    end else begin
      $fatal(1, "[-] Fallo en Paso 2: Estado de telemetría incoherente.");
    end

    // PASO 3: Inyectar Bid de Ruptura y Esperar Recepción Completa
    $display("[+] Enviando Tick 3: BID Ruptura = 77795.0 (> 77792.0 Umbral) [DEBE DISPARAR]...");

    fork
      // Hilo A: Envía el tick Y espera a que la orden regrese completa
      begin
        fork
          send_tick_packet(8'h42, PRICE_BID_HIGH);
          receive_order_packet(rx_magic, rx_action, rx_order_id, rx_timestamp);
        join
        order_received = 1;
      end

      // Hilo B: Watchdog temporal de seguridad (~2.6 ms)
      begin
        #(BYTE_PERIOD_NS * 30);
        if (!order_received) begin
          $fatal(1, "[-] TIMEOUT: La FPGA no emitió el paquete de retorno por TX.");
        end
      end
    join_any
    disable fork;

    $display("\n[!] ===> ¡PAQUETE DE EJECUCIÓN RECIBIDO EN HOST DESDE FPGA! <===");
    $display("    • Sync Magic:     0x%02X (%c)", rx_magic, rx_magic);
    $display("    • Acción Orden:   0x%02X (%c)", rx_action, rx_action);
    $display("    • Order ID:       %0d", rx_order_id);
    $display("    • HW Timestamp:   %0d ciclos (%0d ns)", rx_timestamp, rx_timestamp * 10);

    // PASO 4: Validaciones Estrictas del Protocolo de Retorno (Base 0 -> ID = 0)
    if (rx_magic === 8'h4F && rx_action === 8'h41 && rx_order_id === 32'd0) begin
      $display("\n[+] Integridad de Protocolo: Magic=0x4F ('O'), Action='A' (Sell), ID=0 [OK]");
    end else begin
      $display("[-] Valores recibidos: Magic=0x%02X, Action=0x%02X, ID=%0d", rx_magic, rx_action,
               rx_order_id);
      $fatal(1, "[-] Error de validación en los campos del paquete de orden.");
    end

    $display("\n===================================================================");
    $display("[+] HITO 2.5 VERIFICADO: PIPELINE COMPLETO CERRADO EN SILICIO");
    $display("===================================================================\n");

    #(50_000);
    $finish;
  end

endmodule
