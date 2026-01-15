project('MpiOmpImageProcessing', 'c',
  version : '1.7.0',
  default_options : ['warning_level=3', 'c_std=c99', 'optimization=3'])

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)
add_project_arguments('-march=native', '-DPROJECT_ROOT="' + meson.project_source_root() + '/"', language : 'c')

# Configuration Variables
mpi_processes = '2'
omp_threads = '2'

# Dependencies
omp_dep = dependency('openmp')
mpi_dep = dependency('mpi', language : 'c')
mpirun = find_program('mpirun', required : true)

# Source files
src_files = files(
  'src/benchmark/kernel_run.c',
  'src/benchmark/benchmark_run.c',
  'src/benchmark/benchmark_io.c',
  'src/config/kernel.c',
  'src/config/files.c',
  'src/config/errors.c',
  'src/image/bmp_io.c',
  'src/image/mpi_bmp_io.c',
  'src/image/convolution.c',
  'src/config/file_utils.c',
  'src/tests/validate_images.c',
  'src/main.c',
)

# Include directories
inc_dir = include_directories('include')

# Executable
exe = executable('mpi_omp_convolution',
           src_files,
           include_directories : inc_dir,
           dependencies : [omp_dep, mpi_dep, m_dep],
           install : true)

# Run Targets

# Setup data directory
run_target('setup_project',
  command : ['sh', meson.project_source_root() + '/scripts/setup_project.sh'],
  depends : exe
)

run_target('run_serial',
  command : [exe, '--serial'],
  depends : exe
)

run_target('run_multithreaded',
  command : [exe, '--threads', omp_threads, '--multithreaded'],
  depends : exe
)

run_target('run_distributed',
  command : [mpirun, '-n', mpi_processes, exe, '--threads', omp_threads, '--distributed'],
  depends : exe
)

run_target('run_shared',
  command : [mpirun, '-n', mpi_processes, exe, '--threads', omp_threads, '--shared'],
  depends : exe
)

run_target('run_task_pool',
  command : [mpirun, '-n', mpi_processes, exe, '--threads', omp_threads, '--task_pool'],
  depends : exe
)

run_target('run_all',
  command : [mpirun, '-n', mpi_processes, exe, '--threads', omp_threads, '--all'],
  depends : exe
)

run_target('run_sweep',
  command : ['sh', meson.project_source_root() + '/scripts/run_sweep.sh', mpirun, exe],
  depends : exe
)

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

# Test Targets
test_csv_output = executable('test_csv_output',
  files('src/tests/test_csv_output.c', 'src/config/file_utils.c', 'src/config/errors.c'),
  include_directories : inc_dir,
  dependencies : [omp_dep, mpi_dep, m_dep]
)

test('test_csv_output', test_csv_output)
