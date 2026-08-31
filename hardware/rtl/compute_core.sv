`timescale 1ns / 1ps

module compute_core #(
    // Umbral de disparo configurable (en representacion binaria cruda o pips)
    parameter logic [63:0] TARGET_PRICE_LIMIT = 64'h40F2FE0000000000
) (
    input logic clk,   // Reloj de 100MHz
    input logic rst_n, // Reloj activo de nivel bajo

    // Interfaz de entrada (desde tick_parser_fsm)
    input logic in_tick_valid,  // Pulso estroboscopico de 1 ciclo
    input logic [7:0] in_order_type,  // 'B' (0x42) = Bid, 'A' (0x41) = Ask
    input logic [63:0] in_price,  // Precio IEEE 754 de 64 bits

    // Telemetria del Order Book interno (Top of Book)
    output logic [63:0] out_best_bid,  // Mejor postura de compra actual
    output logic [63:0] out_best_ask,  // Mejor postura de venta actual
    output logic out_book_ready,  // 1 si ya se ha recibido al menos 1 Bid y 1 Ask

    // Interfaz de disparo de ejecución (hacia el bloque TX / Fase 3)
    output logic       out_order_fire,   // Pulso de 1 ciclo para emitir orden
    output logic [7:0] out_order_action  // 'B' = Comprar, 'A' = Vender
);

  // Codigo ASCII para validacion de tipo
  localparam logic [7:0] TYPE_BID = 8'h42;  // 'B'
  localparam logic [7:0] TYPE_ASK = 8'h41;  // 'A'

  // Registros del Top of Book (BBO: Best Bid & Offer)
  logic [63:0] reg_best_bid;
  logic [63:0] reg_best_ask;
  logic reg_has_bid;
  logic reg_has_ask;

  // Asignacion continua para monitorizacion de estado
  assign out_best_bid   = reg_best_bid;
  assign out_best_ask   = reg_best_ask;
  assign out_book_ready = reg_has_bid & reg_has_ask;

  // Logica Secuencial: Actualizacion de Libro y Motor de Disparo
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      reg_best_bid <= 64'd0;
      reg_best_ask <= 64'd0;
      reg_has_bid <= 1'b0;
      reg_has_ask <= 1'b0;
      out_order_fire <= 1'b0;
      out_order_action <= 8'd0;
    end else begin
      // out_order_fire se resetea automáticamente en cada ciclo (patrón strobe)
      out_order_fire <= 1'b0;

      if (in_tick_valid) begin
        // Actualizacion determinista del Orden Book
        if (in_order_type == TYPE_BID) begin
          reg_best_bid <= in_price;
          reg_has_bid  <= 1'b1;
        end else if (in_order_type == TYPE_ASK) begin
          reg_best_ask <= in_price;
          reg_has_ask  <= 1'b1;
        end

        // Evaluación de Estrategia Algorítmica (1 ciclo de latencia)
        // Disparo de Venta (Arbitraje/Take-Profit): Si el Bid supera el límite objetivo
        if (in_order_type == TYPE_BID && (in_price[62:0] > TARGET_PRICE_LIMIT[62:0])) begin
          out_order_fire   <= 1'b1;
          out_order_action <= TYPE_ASK;  // Accion: Vender / Cerrar posicion
        end
        // Disparo de Compra: Si entra un Ask por debajo de una condición de entrada
        // (Extensible para filtros de momentum o arbitraje cruzado Líder-Seguidor)
      end
    end
  end

endmodule
