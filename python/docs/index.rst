Welcome to libcpuid's documentation!
====================================

**libcpuid** provides a Python interface
to the libcpuid C library.

.. code-block:: python

   import sys
   from libcpuid import [
      libcpuid_version,
      get_total_cpus,
      cpuid_present,
      cpu_identify,
      get_cpu_list,
      LibCPUIDError
   ]
   from libcpuid.enums import CPUFeature, CPUVendor

   # print the version of the libcpuid library
   print(libcpuid_version())

   # print the number of CPU cores
   print(get_total_cpus())

   # check if the cpuid instruction is available
   if not cpuid_present():
      print("CPUID instruction is not available")
      sys.exit(1)

   try:
      # run the main CPU identification function
      # and print information about the CPU
      id = cpu_identify()
      print(id.vendor.name)
      print(id.architecture.name)
      print(CPUFeature.FPU in id.flags)
      # print the list of all Intel CPU code names
      print(get_cpu_list(CPUVendor.INTEL))
   except LibCPUIDError as err:
      print(err)
      sys.exit(1)


.. toctree::
   :maxdepth: 2

   Home <self>
   api/libcpuid
   api/info
   api/enums
