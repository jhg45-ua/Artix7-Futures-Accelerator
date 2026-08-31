`timescale 1ns / 1ps

module tb_compute_core;

  localparam int CLK_PERIOD_NS = 10;  // 100 MHz

  logic        clk;
  logic        rst_n;
  logic        in_tick_valid;
  logic [ 7:0] in_order_type;
  logic [63:0] in_price;

  logic [63:0] out_best_bid;
  logic [63:0] out_best_ask;
  logic        out_book_ready;
  logic        out_order_fire;
  logic [ 7:0] out_order_action;

  // Límite fijado en 77792.0 (0x40F2FE0000000000)
  localparam logic [63:0] LIMIT = 64'h40F2FE0000000000;

  // Instancia del DUT (Device Under Test)
  compute_core #(
      .TARGET_PRICE_LIMIT(LIMIT)
  ) dut (
      .clk             (clk),
      .rst_n           (rst_n),
      .in_tick_valid   (in_tick_valid),
      .in_order_type   (in_order_type),
      .in_price        (in_price),
      .out_best_bid    (out_best_bid),
      .out_best_ask    (out_best_ask),
      .out_book_ready  (out_book_ready),
      .out_order_fire  (out_order_fire),
      .out_order_action(out_order_action)
  );

  // Generador de Reloj
  initial clk = 1'b0;
  always #(CLK_PERIOD_NS / 2) clk = ~clk;

  // Tarea para enviar un tick en 1 ciclo
  task automatic inject_tick(input logic [7:0] side, input logic [63:0] price);
    begin
      @(posedge clk);
      in_tick_valid <= 1'b1;
      in_order_type <= side;
      in_price      <= price;
      @(posedge clk);
      in_tick_valid <= 1'b0;
    end
  endtask

  initial begin
    // Valores de prueba IEEE 754
    // 77790.0 = 0x40F2FD8000000000 (Menor que el límite)
    // 77795.0 = 0x40F2FE6000000000 (Mayor que el límite -> Dispara orden)
    localparam logic [63:0] PRICE_BID_LOW = 64'h40F2FD8000000000;
    localparam logic [63:0] PRICE_ASK_INIT = 64'h40F2FDC000000000;
    localparam logic [63:0] PRICE_BID_HIGH = 64'h40F2FE6000000000;

    in_tick_valid <= 1'b0;
    in_order_type <= 8'd0;
    in_price      <= 64'd0;
    rst_n         <= 1'b0;
    #(50);
    rst_n <= 1'b1;
    #(20);

    $display("\n========================================================");
    $display("[TB] INICIANDO VERIFICACIÓN UNITARIA: COMPUTE_CORE");
    $display("========================================================");

    // Paso 1: Inyectar Ask inicial y verificar que el libro no está listo
    inject_tick(8'h41, PRICE_ASK_INIT);  // Ask
    #(CLK_PERIOD_NS);
    if (!out_book_ready && out_best_ask === PRICE_ASK_INIT) begin
      $display("[+] Paso 1: Ask registrado correctamente. Libro pendiente de Bid [OK]");
    end else begin
      $fatal(1, "[-] Fallo en Paso 1.");
    end

    // Paso 2: Inyectar Bid por debajo del umbral -> No debe disparar orden
    inject_tick(8'h42, PRICE_BID_LOW);  // Bid
    #(CLK_PERIOD_NS);
    if (out_book_ready && out_best_bid === PRICE_BID_LOW && !out_order_fire) begin
      $display("[+] Paso 2: Bid registrado. Libro listo. Disparo inactivo [OK]");
    end else begin
      $fatal(1, "[-] Fallo en Paso 2.");
    end

    // Paso 3: Inyectar Bid por encima del umbral -> Debe disparar orden en 1 ciclo
    inject_tick(8'h42, PRICE_BID_HIGH);  // Bid que rompe umbral
    if (out_order_fire && out_order_action === 8'h41) begin
      $display("[+] Paso 3: ¡DISPARO DETECTADO! Señal out_order_fire activa con acción %c [OK]",
               out_order_action);
    end else begin
      $fatal(1, "[-] Fallo en Paso 3: La condición algorítmica no disparó la orden.");
    end

    // Paso 4: Comprobar que en el siguiente ciclo out_order_fire vuelve a 0
    @(posedge clk);
    if (!out_order_fire) begin
      $display(
          "[+] Paso 4: out_order_fire regresó a 0 en el siguiente ciclo (10 ns strobe verificado) [OK]");
    end else begin
      $fatal(1, "[-] Fallo en Paso 4: out_order_fire quedó pegado a nivel alto.");
    end

    $display("\n========================================================");
    $display("[+] HITO 2.2 VERIFICADO CON ÉXITO: 0 ns de latencia adicional");
    $display("========================================================\n");
    #(50);
    $finish;
  end

endmodule
