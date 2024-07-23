Welcome to libcpuid's documentation!
====================================

**libcpuid** provides a Python interface
to the libcpuid C library.

.. code-block:: python

   import sys
   import libcpuid
   from libcpuid.errors import LibcpuidError
   from libcpuid.info import CPUInfo
   from libcpuid.enums import CPUFeature, CPUVendor

   # print the version of the libcpuid library
   print(libcpuid.version())

   # print the number of CPU cores
   print(libcpuid.get_total_cpus())

   # check if the cpuid instruction is available
   if not libcpuid.cpuid_present():
      print("CPUInfo instruction is not available")
      sys.exit(1)

   try:
      # identify the current CPU and print some
      # information about it
      cpu_info = CPUInfo.from_current_cpu()
      print(cpu_info.vendor)
      print(cpu_info.architecture)
      print(CPUFeature.FPU in cpu_info.features)

      # print the list of all Intel CPU code names
      print(libcpuid.get_cpu_list(CPUVendor.INTEL))
   except LibcpuidError as err:
      print(err.message)
      sys.exit(1)


.. toctree::
   :maxdepth: 2

   Home <self>
   api/libcpuid
   api/info
   api/msr
   api/enums
   api/raw
   api/clock
   api/errors
