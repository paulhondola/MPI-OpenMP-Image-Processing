project('MpiOmpImageProcessing', 'c',
  version : '0.9.0',
  default_options : ['warning_level=everything', 'c_std=c99', 'optimization=3', 'b_lto=true'])

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)
add_project_arguments('-march=native', '-DPROJECT_ROOT="' + meson.project_source_root() + '/"', language : 'c')

# Configuration Variables
mpi_processes = '8'
omp_threads = '8'

# Dependencies
omp_dep = dependency('openmp')
mpi_dep = dependency('mpi', language : 'c')

# Source files
src_files = files(
  'src/benchmark/kernel_run.c',
  'src/benchmark/benchmark_run.c',
  'src/benchmark/benchmark_io.c',
  'src/config/kernel.c',
  'src/config/files.c',
  'src/main.c',
  'src/errors/errors.c',
  'src/bmp/bmp_io.c',
  'src/bmp/mpi_bmp_io.c',
  'src/convolution/convolution.c',
  'src/file_utils/file_utils.c'
)

# Include directories
inc_dir = include_directories('src')

# Executable
exe = executable('mpi_omp_convolution',
           src_files,
           include_directories : inc_dir,
           dependencies : [omp_dep, mpi_dep, m_dep],
           install : true)

# Run Targets
# Serial
run_target('run_serial',
  command : [exe, '-serial'],
  depends : exe
)

# Multithreaded
run_target('run_multithreaded',
  command : [exe, '-threads', omp_threads, '-multithreaded'],
   depends : exe
)

# Distributed
# We use 'mpirun' (or mpiexec) which should be available if MPI dependency is found.
# Meson doesn't have a built-in 'mpirun' variable exposed easily without finding program.
mpirun = find_program('mpirun', required : true)

run_target('run_distributed',
  command : [mpirun, '-n', mpi_processes, exe, '-threads', omp_threads, '-distributed'],
   depends : exe
)

'''
# Shared
run_target('run_shared',
  command : [mpirun, '-n', mpi_processes, exe, '-threads', omp_threads, '-shared'],
   depends : exe
)

# Task Pool
run_target('run_task_pool',
  command : [mpirun, '-n', mpi_processes, exe, '-threads', omp_threads, '-task_pool'],
   depends : exe
)
'''

# All
run_target('run_all',
  command : [mpirun, '-n', mpi_processes, exe, '-threads', omp_threads, '-all'],
   depends : exe
)

# Clean Targets
# Cleans all generated images except the 'base' directory
run_target('clean_images',
  command : ['sh', meson.project_source_root() + '/scripts/clear_images.sh']
)

# Cleans all benchmark CSV data
run_target('clean_data',
  command : ['sh', meson.project_source_root() + '/scripts/clear_data.sh']
)

# Kills the running processes
run_target('kill',
  command : ['sh', meson.project_source_root() + '/scripts/kill_process.sh']
)

# Plots the data
python3 = find_program('python3', required : true)
run_target('plot',
  command : [
    python3, 
    meson.project_source_root() + '/scripts/plot.py',
    '--data-file', meson.project_source_root() + '/data/chronos/speedups_data.csv',
    '--output-dir', meson.project_source_root() + '/data/plots'
  ]
)

# Runs the sweep script
run_target('run_sweep',
  command : ['sh', meson.project_source_root() + '/scripts/run_sweep.sh', mpirun, exe],
  depends : exe
)
