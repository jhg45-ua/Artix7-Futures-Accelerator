`timescale 1ns / 1ps

module uart_tx #(
    parameter int CLKS_PER_BIT = 868
) (
    input  logic       clk,       // Reloj de 100 MHz (10 ns)
    input  logic       rst_n,     // Reset activo a nivel bajo
    input  logic       tx_start,  // Pulso de 1 ciclo para iniciar transmisión
    input  logic [7:0] tx_data,   // Byte a transmitir
    output logic       tx,        // Pin físico de salida serie
    output logic       tx_busy,   // 1 si el transmisor está ocupado enviando
    output logic       tx_done    // Pulso estroboscópico de 1 ciclo al completar el byte
);

  typedef enum logic [1:0] {
    IDLE      = 2'b00,
    START_BIT = 2'b01,
    DATA_BITS = 2'b10,
    STOP_BIT  = 2'b11
  } tx_state_t;

  tx_state_t state;
  logic [15:0] clk_count;
  logic [2:0] bit_index;
  logic [7:0] reg_tx_byte;

  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      state       <= IDLE;
      clk_count   <= 16'd0;
      bit_index   <= 3'd0;
      reg_tx_byte <= 8'd0;
      tx          <= 1'b1;  // Línea UART en reposo = Nivel alto (1)
      tx_busy     <= 1'b0;
      tx_done     <= 1'b0;
    end else begin
      tx_done <= 1'b0;  // Valor por defecto tipo strobe

      case (state)
        IDLE: begin
          tx        <= 1'b1;
          tx_busy   <= 1'b0;
          clk_count <= 16'd0;
          bit_index <= 3'd0;

          if (tx_start) begin
            reg_tx_byte <= tx_data;
            tx_busy     <= 1'b1;
            state       <= START_BIT;
          end
        end

        START_BIT: begin
          tx <= 1'b0;  // Bit de Start = Nivel bajo (0)

          if (clk_count < CLKS_PER_BIT - 1) begin
            clk_count <= clk_count + 1'b1;
          end else begin
            clk_count <= 16'd0;
            state     <= DATA_BITS;
          end
        end

        DATA_BITS: begin
          tx <= reg_tx_byte[bit_index];  // Emisión LSB primero

          if (clk_count < CLKS_PER_BIT - 1) begin
            clk_count <= clk_count + 1'b1;
          end else begin
            clk_count <= 16'd0;
            if (bit_index < 3'd7) begin
              bit_index <= bit_index + 1'b1;
            end else begin
              bit_index <= 3'd0;
              state     <= STOP_BIT;
            end
          end
        end

        STOP_BIT: begin
          tx <= 1'b1;  // Bit de Stop = Nivel alto (1)

          if (clk_count < CLKS_PER_BIT - 1) begin
            clk_count <= clk_count + 1'b1;
          end else begin
            clk_count <= 16'd0;
            tx_done   <= 1'b1;  // Notifica que el byte ha salido completamente
            tx_busy   <= 1'b0;
            state     <= IDLE;
          end
        end

        default: state <= IDLE;
      endcase
    end
  end


endmodule
