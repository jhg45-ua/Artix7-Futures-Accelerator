# ==============================================================================
# Script de Reconstrucción del Proyecto Vivado
# Target: Nexys A7-100T (xc7a100tcsg324-1)
# ==============================================================================

set project_name "hft_fpga_pipeline"
set origin_dir   [file normalize [file dirname [info script]]]
set ws_dir       "$origin_dir/workspace"

# Crear proyecto limpio en hardware/workspace/
file mkdir $ws_dir
create_project -force $project_name $ws_dir -part xc7a100tcsg324-1

# Configuracion de propiedades globales
set_property target_language Verilog [current_project]
set_property simulator_language Mixed [current_project]
set_property default_lib xil_defaultlib [current_project]

# Importar archivos RTL (SystemVerilog)
set rtl_files [glob -nocomplain "$origin_dir/rtl/*.sv"]
if {[llength $rtl_files] > 0} {
    add_files -norecurse -fileset sources_1 $rtl_files
    set_property file_type "SystemVerilog" [get_files $rtl_files]
    set_property top hft_top [current_fileset]
}

# Importar Testbenches (SystemVerilog)
set tb_files [glob -nocomplain "$origin_dir/tb/*.sv"]
if {[llength $tb_files] > 0} {
    add_files -norecurse -fileset sim_1 $tb_files
    set_property file_type "SystemVerilog" [get_files $tb_files]
    if {[file exists "$origin_dir/tb/tb_hft_pipeline.sv"]} {
        set_property top tb_hft_pipeline [get_filesets sim_1]
    }
}

# Importar Restricciones Físicas (.xdc)
set xdc_files [glob -nocomplain "$origin_dir/constraints/*.xdc"]
if {[llength $xdc_files] > 0} {
    add_files -norecurse -fileset constrs_1 $xdc_files
}

puts "\n\[+\] Proyecto generado en: $ws_dir/$project_name.xpr\n"