`timescale 1ns / 1ps

module tb_tx_subsystem;

  localparam int CLK_PERIOD_NS = 10;
  localparam int CLKS_PER_BIT  = 868;
  localparam int BIT_PERIOD_NS = CLKS_PER_BIT * CLK_PERIOD_NS;  // 8680 ns

  logic       clk;
  logic       rst_n;
  logic       in_order_fire;
  logic [7:0] in_order_action;
  logic       tx_serial;
  logic       tx_busy;
  logic       tx_done;
  logic       tx_start;
  logic [7:0] tx_data;
  logic       out_packet_busy;

  // Instancia del Empaquetador
  order_packet_builder u_builder (
      .clk            (clk),
      .rst_n          (rst_n),
      .in_order_fire  (in_order_fire),
      .in_order_action(in_order_action),
      .tx_busy        (tx_busy),
      .tx_done        (tx_done),
      .tx_start       (tx_start),
      .tx_data        (tx_data),
      .out_packet_busy(out_packet_busy)
  );

  // Instancia del Transmisor UART
  uart_tx #(
      .CLKS_PER_BIT(CLKS_PER_BIT)
  ) u_uart_tx (
      .clk     (clk),
      .rst_n   (rst_n),
      .tx_start(tx_start),
      .tx_data (tx_data),
      .tx      (tx_serial),
      .tx_busy (tx_busy),
      .tx_done (tx_done)
  );

  // Generador de Reloj a 100 MHz
  initial clk = 1'b0;
  always #(CLK_PERIOD_NS / 2) clk = ~clk;

  // Tarea para recibir un byte desde la línea serie simulada
  task automatic receive_uart_byte(output logic [7:0] captured_byte);
    begin
      @(negedge tx_serial);  // Detección del Start Bit (1 -> 0)
      #(BIT_PERIOD_NS + (BIT_PERIOD_NS / 2));  // Saltar al centro del bit 0

      for (int i = 0; i < 8; i++) begin
        captured_byte[i] = tx_serial;
        #(BIT_PERIOD_NS);
      end
    end
  endtask

  initial begin
    logic [7:0] received_packet[0:9];

    in_order_fire   <= 1'b0;
    in_order_action <= 8'd0;
    rst_n           <= 1'b0;
    #(50);
    rst_n <= 1'b1;
    #(100);

    $display("\n========================================================");
    $display("[TB] INICIANDO VERIFICACIÓN UNITARIA: SUBSISTEMA TX");
    $display("========================================================");

    // Disparo de orden de Compra ('B' = 0x42)
    @(posedge clk);
    in_order_fire   <= 1'b1;
    in_order_action <= 8'h42;
    @(posedge clk);
    in_order_fire <= 1'b0;

    // Monitor en paralelo para capturar los 10 bytes emitidos por tx_serial
    for (int b = 0; b < 10; b++) begin
      receive_uart_byte(received_packet[b]);
      $display("[+] Byte [%0d/9] Recibido por UART TX: 0x%02X (%c)", b, received_packet[b],
               (received_packet[b] >= 32 && received_packet[b] <= 126) ? received_packet[b] : ".");
    end

    // Validaciones
    if (received_packet[0] === 8'h4F && received_packet[1] === 8'h42) begin
      $display("[+] Verificación de Cabecera: Magic=0x4F ('O'), Action='B' [OK]");
    end else begin
      $fatal(1, "[-] Error en cabecera de paquete.");
    end

    $display("\n========================================================");
    $display("[+] HITO 2.3 VERIFICADO CON ÉXITO: 10 Bytes Serializados");
    $display("========================================================\n");
    #(10_000);
    $finish;
  end

endmodule
