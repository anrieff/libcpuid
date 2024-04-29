Welcome to libcpuid's documentation!
====================================

**libcpuid** provides a Python interface
to the libcpuid C library.

.. code-block:: python

   import sys
   import libcpuid

   # Initialize the libcpuid library
   try:
      lib = libcpuid.LibCPUID()
   except libcpuid.LibCPUIDError as err:
      print(err)
      sys.exit(1)

   # print the version of the libcpuid library
   print(lib.version())

   # print the number of CPU cores
   print(lib.get_total_cpus())

   # check if the cpuid instruction is available
   if not lib.cpuid_present():
      print("CPUID instruction is not available")
      sys.exit(1)

   try:
      # run the main CPU identification function
      # and print information about the CPU
      cpuid = lib.cpu_identify()
      print(cpuid.vendor.name)
      print(cpuid.architecture.name)
      print(cpuid.has_feature(libcpuid.enums.CPUFeature.FPU))
      # print the list of all Intel CPU code names
      print(lib.get_cpu_list(libcpuid.enums.CPUVendor.INTEL))
   except libcpuid.LibCPUIDError as err:
      print(err)
      sys.exit(1)


.. toctree::
   :maxdepth: 2

   Home <self>
   api/libcpuid
   api/cpuid
   api/enums
