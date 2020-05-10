/*
 * Copyright 2008  Veselin Georgiev,
 * anrieffNOSPAM @ mgail_DOT.com (convert to gmail)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <string.h>
#include <ctype.h>
#include "libcpuid.h"
#include "libcpuid_util.h"
#include "libcpuid_internal.h"
#include "recog_intel.h"

const struct intel_bcode_str { intel_code_t code; char *str; } intel_bcode_str[] = {
	#define CODE(x) { x, #x }
	#define CODE2(x, y) CODE(x)
	#include "intel_code_t.h"
	#undef CODE
};

typedef struct {
	int code;
	uint64_t bits;
} intel_code_and_bits_t;

enum _intel_model_t {
	UNKNOWN = -1,
	_3000 = 100,
	_3100,
	_3200,
	X3200,
	_3300,
	X3300,
	_5100,
	_5200,
	_5300,
	_5400,
	_2xxx, /* Core i[357] 2xxx */
	_3xxx, /* Core i[357] 3xxx */
};
typedef enum _intel_model_t intel_model_t;

const struct match_entry_t cpudb_intel[] = {
//     F   M   S  EF  EM  #cores  L2$   L3$  BC  ModelBits     ModelCode  Name
	{ -1, -1, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Unknown Intel CPU"       },

	/* i486 */
	{  4, -1, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Unknown i486"            },
	{  4,  0, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "i486 DX-25/33"           },
	{  4,  1, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "i486 DX-50"              },
	{  4,  2, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "i486 SX"                 },
	{  4,  3, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "i486 DX2"                },
	{  4,  4, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "i486 SL"                 },
	{  4,  5, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "i486 SX2"                },
	{  4,  7, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "i486 DX2 WriteBack"      },
	{  4,  8, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "i486 DX4"                },
	{  4,  9, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "i486 DX4 WriteBack"      },

	/* All Pentia:
	   Pentium 1 */
	{  5, -1, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Unknown Pentium"         },
	{  5,  0, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Pentium A-Step"          },
	{  5,  1, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Pentium 1 (0.8u)"        },
	{  5,  2, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Pentium 1 (0.35u)"       },
	{  5,  3, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Pentium OverDrive"       },
	{  5,  4, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Pentium 1 (0.35u)"       },
	{  5,  7, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Pentium 1 (0.35u)"       },
	{  5,  8, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Pentium MMX (0.25u)"     },

	/* Pentium 2 / 3 / M / Conroe / whatsnext - all P6 based. */
	{  6, -1, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Unknown P6"              },
	{  6,  0, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Pentium Pro"             },
	{  6,  1, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Pentium Pro"             },
	{  6,  3, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Pentium II (Klamath)"    },
	{  6,  5, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Pentium II (Deschutes)"  },
	{  6,  5, -1, -1, -1,   1,    -1,    -1, NC, MOBILE_|PENTIUM_,   0, "Mobile Pentium II (Tonga)"},
	{  6,  6, -1, -1, -1,   1,    -1,    -1, NC,0              ,     0, "Pentium II (Dixon)"      },

	{  6,  3, -1, -1, -1,   1,    -1,    -1, NC, XEON_         ,     0, "P-II Xeon (Klamath)"     },
	{  6,  5, -1, -1, -1,   1,    -1,    -1, NC, XEON_         ,     0, "P-II Xeon (Drake)"       },
	{  6,  6, -1, -1, -1,   1,    -1,    -1, NC, XEON_         ,     0, "P-II Xeon (Dixon)"       },

	{  6,  5, -1, -1, -1,   1,    -1,    -1, NC, CELERON_      ,     0, "P-II Celeron (Covington)" },
	{  6,  6, -1, -1, -1,   1,    -1,    -1, NC, CELERON_      ,     0, "P-II Celeron (Mendocino)" },

	/* -------------------------------------------------- */

	{  6,  7, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Pentium III (Katmai)"    },
	{  6,  8, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Pentium III (Coppermine)"},
	{  6, 10, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Pentium III (Coppermine)"},
	{  6, 11, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Pentium III (Tualatin)"  },
	{  6, 11, -1, -1, -1,   1,   512,    -1, NC, 0             ,     0, "Pentium III (Tualatin)"  },

	{  6,  7, -1, -1, -1,   1,    -1,    -1, NC, XEON_         ,     0, "P-III Xeon (Tanner)"     },
	{  6,  8, -1, -1, -1,   1,    -1,    -1, NC, XEON_         ,     0, "P-III Xeon (Cascades)"   },
	{  6, 10, -1, -1, -1,   1,    -1,    -1, NC, XEON_         ,     0, "P-III Xeon (Cascades)"   },
	{  6, 11, -1, -1, -1,   1,    -1,    -1, NC, XEON_         ,     0, "P-III Xeon (Tualatin)"   },

	{  6,  7, -1, -1, -1,   1,   128,    -1, NC, CELERON_      ,     0, "P-III Celeron (Katmai)"     },
	{  6,  8, -1, -1, -1,   1,   128,    -1, NC, CELERON_      ,     0, "P-III Celeron (Coppermine)" },
	{  6, 10, -1, -1, -1,   1,   128,    -1, NC, CELERON_      ,     0, "P-III Celeron (Coppermine)" },
	{  6, 11, -1, -1, -1,   1,   256,    -1, NC, CELERON_      ,     0, "P-III Celeron (Tualatin)"   },

	/* Netburst based (Pentium 4 and later)
	   classic P4s */
	{ 15, -1, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Unknown Pentium 4"       },
	{ 15, -1, -1, 15, -1,   1,    -1,    -1, NC, CELERON_      ,     0, "Unknown P-4 Celeron"     },
	{ 15, -1, -1, 15, -1,   1,    -1,    -1, NC, XEON_         ,     0, "Unknown Xeon"            },

	{ 15,  0, -1, 15, -1,   1,    -1,    -1, NC, PENTIUM_      ,     0, "Pentium 4 (Willamette)"  },
	{ 15,  1, -1, 15, -1,   1,    -1,    -1, NC, PENTIUM_      ,     0, "Pentium 4 (Willamette)"  },
	{ 15,  2, -1, 15, -1,   1,    -1,    -1, NC, PENTIUM_      ,     0, "Pentium 4 (Northwood)"   },
	{ 15,  3, -1, 15, -1,   1,    -1,    -1, NC, PENTIUM_      ,     0, "Pentium 4 (Prescott)"    },
	{ 15,  4, -1, 15, -1,   1,    -1,    -1, NC, PENTIUM_      ,     0, "Pentium 4 (Prescott)"    },
	{ 15,  6, -1, 15, -1,   1,    -1,    -1, NC, PENTIUM_      ,     0, "Pentium 4 (Cedar Mill)"  },
	{ 15,  0, -1, 15, -1,   1,    -1,    -1, NC, MOBILE_|PENTIUM_,   0, "Mobile P-4 (Willamette)" },
	{ 15,  1, -1, 15, -1,   1,    -1,    -1, NC, MOBILE_|PENTIUM_,   0, "Mobile P-4 (Willamette)" },
	{ 15,  2, -1, 15, -1,   1,    -1,    -1, NC, MOBILE_|PENTIUM_,   0, "Mobile P-4 (Northwood)"  },
	{ 15,  3, -1, 15, -1,   1,    -1,    -1, NC, MOBILE_|PENTIUM_,   0, "Mobile P-4 (Prescott)"   },
	{ 15,  4, -1, 15, -1,   1,    -1,    -1, NC, MOBILE_|PENTIUM_,   0, "Mobile P-4 (Prescott)"   },
	{ 15,  6, -1, 15, -1,   1,    -1,    -1, NC, MOBILE_|PENTIUM_,   0, "Mobile P-4 (Cedar Mill)" },

	/* server CPUs */
	{ 15,  0, -1, 15, -1,   1,    -1,    -1, NC, XEON_         ,     0, "Xeon (Foster)"           },
	{ 15,  1, -1, 15, -1,   1,    -1,    -1, NC, XEON_         ,     0, "Xeon (Foster)"           },
	{ 15,  2, -1, 15, -1,   1,    -1,    -1, NC, XEON_         ,     0, "Xeon (Prestonia)"        },
	{ 15,  2, -1, 15, -1,   1,    -1,    -1, NC, XEON_|_MP_    ,     0, "Xeon (Gallatin)"         },
	{ 15,  3, -1, 15, -1,   1,    -1,    -1, NC, XEON_         ,     0, "Xeon (Nocona)"           },
	{ 15,  4, -1, 15, -1,   1,    -1,    -1, NC, XEON_         ,     0, "Xeon (Nocona)"           },
	{ 15,  4, -1, 15, -1,   1,    -1,    -1, IRWIN, XEON_      ,     0, "Xeon (Irwindale)"        },
	{ 15,  4, -1, 15, -1,   1,    -1,    -1, NC, XEON_|_MP_    ,     0, "Xeon (Cranford)"         },
	{ 15,  4, -1, 15, -1,   1,    -1,    -1, POTOMAC, XEON_    ,     0, "Xeon (Potomac)"          },
	{ 15,  6, -1, 15, -1,   1,    -1,    -1, NC, XEON_         ,     0, "Xeon (Dempsey)"          },

	/* Pentium Ds */
	{ 15,  4,  4, 15, -1,   1,    -1,    -1, NC, 0             ,     0, "Pentium D (SmithField)"  },
	{ 15,  4, -1, 15, -1,   1,    -1,    -1, PENTIUM_D, 0      ,     0, "Pentium D (SmithField)"  },
	{ 15,  4,  7, 15, -1,   1,    -1,    -1, NC, 0             ,     0, "Pentium D (SmithField)"  },
	{ 15,  6, -1, 15, -1,   1,    -1,    -1, PENTIUM_D, 0      ,     0, "Pentium D (Presler)"     },

	/* Celeron and Celeron Ds */
	{ 15,  1, -1, 15, -1,   1,    -1,    -1, NC, CELERON_      ,     0, "P-4 Celeron (Willamette)"   },
	{ 15,  2, -1, 15, -1,   1,    -1,    -1, NC, CELERON_      ,     0, "P-4 Celeron (Northwood)"    },
	{ 15,  3, -1, 15, -1,   1,    -1,    -1, NC, CELERON_      ,     0, "P-4 Celeron D (Prescott)"   },
	{ 15,  4, -1, 15, -1,   1,    -1,    -1, NC, CELERON_      ,     0, "P-4 Celeron D (Prescott)"   },
	{ 15,  6, -1, 15, -1,   1,    -1,    -1, NC, CELERON_      ,     0, "P-4 Celeron D (Cedar Mill)" },

	/* -------------------------------------------------- */
	/* Intel Core microarchitecture - P6-based */

	{  6,  9, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Unknown Pentium M"          },
	{  6,  9, -1, -1, -1,   1,    -1,    -1, PENTIUM_M, 0      ,     0, "Unknown Pentium M"          },
	{  6,  9, -1, -1, -1,   1,    -1,    -1, NC, PENTIUM_      ,     0, "Pentium M (Banias)"         },
	{  6,  9, -1, -1, -1,   1,    -1,    -1, PENTIUM_M, 0      ,     0, "Pentium M (Banias)"         },
	{  6,  9, -1, -1, -1,   1,    -1,    -1, NC, CELERON_      ,     0, "Celeron M"                  },
	{  6, 13, -1, -1, -1,   1,    -1,    -1, NC, PENTIUM_      ,     0, "Pentium M (Dothan)"         },
	{  6, 13, -1, -1, -1,   1,    -1,    -1, PENTIUM_M, 0      ,     0, "Pentium M (Dothan)"         },
	{  6, 13, -1, -1, -1,   1,    -1,    -1, NC, CELERON_      ,     0, "Celeron M"                  },

	{  6, 12, -1, -1, -1,  -1,    -1,    -1, NC, ATOM_         ,     0, "Unknown Atom"               },
	{  6, 12, -1, -1, -1,  -1,    -1,    -1, DIAMONDVILLE,ATOM_,     0, "Atom (Diamondville)"        },
	{  6, 12, -1, -1, -1,  -1,    -1,    -1, SILVERTHORNE,ATOM_,     0, "Atom (Silverthorne)"        },
	{  6, 12, -1, -1, -1,  -1,    -1,    -1, CEDARVIEW, ATOM_  ,     0, "Atom (Cedarview)"           },
	{  6,  6, -1, -1, -1,  -1,    -1,    -1, CEDARVIEW, ATOM_  ,     0, "Atom (Cedarview)"           },
	{  6, 12, -1, -1, -1,  -1,    -1,    -1, PINEVIEW, ATOM_   ,     0, "Atom (Pineview)"            },

	/* -------------------------------------------------- */

	{  6, 14, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Unknown Yonah"             },
	{  6, 14, -1, -1, -1,   1,    -1,    -1, CORE_SOLO, 0      ,     0, "Yonah (Core Solo)"         },
	{  6, 14, -1, -1, -1,   2,    -1,    -1, CORE_DUO, 0       ,     0, "Yonah (Core Duo)"          },
	{  6, 14, -1, -1, -1,   1,    -1,    -1, CORE_SOLO, MOBILE_,     0, "Yonah (Core Solo)"         },
	{  6, 14, -1, -1, -1,   2,    -1,    -1, CORE_DUO , MOBILE_,     0, "Yonah (Core Duo)"          },
	{  6, 14, -1, -1, -1,   1,    -1,    -1, CORE_SOLO, 0      ,     0, "Yonah (Core Solo)"         },

	{  6, 15, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Unknown Core 2"            },
	{  6, 15, -1, -1, -1,   2,  4096,    -1, CORE_DUO, 0       ,     0, "Conroe (Core 2 Duo)"       },
	{  6, 15, -1, -1, -1,   2,  1024,    -1, CORE_DUO, 0       ,     0, "Conroe (Core 2 Duo) 1024K" },
	{  6, 15, -1, -1, -1,   2,   512,    -1, CORE_DUO, 0       ,     0, "Conroe (Core 2 Duo) 512K"  },
	{  6, 15, -1, -1, -1,   4,    -1,    -1, QUAD_CORE, 0      ,     0, "Kentsfield (Core 2 Quad)"  },
	{  6, 15, -1, -1, -1,   4,  4096,    -1, QUAD_CORE, 0      ,     0, "Kentsfield (Core 2 Quad)"  },
	{  6, 15, -1, -1, -1, 400,    -1,    -1, MORE_THAN_QUADCORE, 0,  0, "More than quad-core"       },
	{  6, 15, -1, -1, -1,   2,  2048,    -1, CORE_DUO, 0       ,     0, "Allendale (Core 2 Duo)"    },
	{  6, 15, -1, -1, -1,   2,    -1,    -1, MOBILE_CORE_DUO, 0,     0, "Merom (Core 2 Duo)"        },
	{  6, 15, -1, -1, -1,   2,  2048,    -1, MEROM, 0          ,     0, "Merom (Core 2 Duo) 2048K"  },
	{  6, 15, -1, -1, -1,   2,  4096,    -1, MEROM, 0          ,     0, "Merom (Core 2 Duo) 4096K"  },

	{  6, 15, -1, -1, 15,   1,    -1,    -1, NC, CELERON_      ,     0, "Conroe-L (Celeron)"        },
	{  6,  6, -1, -1, 22,   1,    -1,    -1, NC, CELERON_      ,     0, "Conroe-L (Celeron)"        },
	{  6, 15, -1, -1, 15,   2,    -1,    -1, NC, CELERON_      ,     0, "Conroe-L (Allendale)"      },
	{  6,  6, -1, -1, 22,   2,    -1,    -1, NC, CELERON_      ,     0, "Conroe-L (Allendale)"      },


	{  6,  6, -1, -1, 22,   1,    -1,    -1, NC, 0             ,     0, "Unknown Core ?"           },
	{  6,  7, -1, -1, 23,   1,    -1,    -1, NC, 0             ,     0, "Unknown Core ?"           },
	{  6,  6, -1, -1, 22, 400,    -1,    -1, MORE_THAN_QUADCORE, 0,  0, "More than quad-core"      },
	{  6,  7, -1, -1, 23, 400,    -1,    -1, MORE_THAN_QUADCORE, 0,  0, "More than quad-core"      },

	{  6,  7, -1, -1, 23,   1,    -1,    -1, CORE_SOLO         , 0,  0, "Unknown Core 45nm"        },
	{  6,  7, -1, -1, 23,   1,    -1,    -1, CORE_DUO          , 0,  0, "Unknown Core 45nm"        },
	{  6,  7, -1, -1, 23,   2,  1024,    -1, WOLFDALE          , 0,  0, "Celeron Wolfdale 1M"      },
	{  6,  7, -1, -1, 23,   2,  2048,    -1, WOLFDALE          , 0,  0, "Wolfdale (Core 2 Duo) 2M" },
	{  6,  7, -1, -1, 23,   2,  3072,    -1, WOLFDALE          , 0,  0, "Wolfdale (Core 2 Duo) 3M" },
	{  6,  7, -1, -1, 23,   2,  6144,    -1, WOLFDALE          , 0,  0, "Wolfdale (Core 2 Duo) 6M" },
	{  6,  7, -1, -1, 23,   1,    -1,    -1, MOBILE_CORE_DUO   , 0,  0, "Penryn (Core 2 Duo)"      },
	{  6,  7, -1, -1, 23,   2,  1024,    -1, PENRYN            , 0,  0, "Penryn (Core 2 Duo)"      },
	{  6,  7, -1, -1, 23,   2,  3072,    -1, PENRYN            , 0,  0, "Penryn (Core 2 Duo) 3M"   },
	{  6,  7, -1, -1, 23,   2,  6144,    -1, PENRYN            , 0,  0, "Penryn (Core 2 Duo) 6M"   },
	{  6,  7, -1, -1, 23,   4,  2048,    -1, NC                , 0,  0, "Yorkfield (Core 2 Quad) 2M"},
	{  6,  7, -1, -1, 23,   4,  3072,    -1, NC                , 0,  0, "Yorkfield (Core 2 Quad) 3M"},
	{  6,  7, -1, -1, 23,   4,  6144,    -1, NC                , 0,  0, "Yorkfield (Core 2 Quad) 6M"},

	/* Core microarchitecture-based Xeons: */
	{  6, 14, -1, -1, 14,   1,    -1,    -1, NC, XEON_         ,     0, "Xeon LV"                  },
	{  6, 15, -1, -1, 15,   2,  4096,    -1, NC, XEON_         , _5100, "Xeon (Woodcrest)"         },
	{  6, 15, -1, -1, 15,   2,  2048,    -1, NC, XEON_         , _3000, "Xeon (Conroe/2M)"         },
	{  6, 15, -1, -1, 15,   2,  4096,    -1, NC, XEON_         , _3000, "Xeon (Conroe/4M)"         },
	{  6, 15, -1, -1, 15,   4,  4096,    -1, NC, XEON_         , X3200, "Xeon (Kentsfield)"        },
	{  6, 15, -1, -1, 15,   4,  4096,    -1, NC, XEON_         , _5300, "Xeon (Clovertown)"        },
	{  6,  7, -1, -1, 23,   2,  6144,    -1, NC, XEON_         , _3100, "Xeon (Wolfdale)"          },
	{  6,  7, -1, -1, 23,   2,  6144,    -1, NC, XEON_         , _5200, "Xeon (Wolfdale DP)"       },
	{  6,  7, -1, -1, 23,   4,  6144,    -1, NC, XEON_         , _5400, "Xeon (Harpertown)"        },
	{  6,  7, -1, -1, 23,   4,  3072,    -1, NC, XEON_         , X3300, "Xeon (Yorkfield/3M)"      },
	{  6,  7, -1, -1, 23,   4,  6144,    -1, NC, XEON_         , X3300, "Xeon (Yorkfield/6M)"      },

	/* Nehalem CPUs (45nm): */
	{  6, 10, -1, -1, 26,   4,    -1,    -1, GAINESTOWN, XEON_ ,     0, "Gainestown (Xeon)"        },
	{  6, 10, -1, -1, 26,   4,    -1,  4096, GAINESTOWN, XEON_ ,     0, "Gainestown 4M (Xeon)"     },
	{  6, 10, -1, -1, 26,   4,    -1,  8192, GAINESTOWN, XEON_ ,     0, "Gainestown 8M (Xeon)"     },
	{  6, 10, -1, -1, 26,   4,    -1,    -1, NC, XEON_|_7      ,     0, "Bloomfield (Xeon)"        },
	{  6, 10, -1, -1, 26,   4,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Bloomfield (Core i7)"     },
	{  6, 10, -1, -1, 30,   4,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Lynnfield (Core i7)"      },
	{  6,  5, -1, -1, 37,   4,    -1,  8192, NC, CORE_|_I_|_5  ,     0, "Lynnfield (Core i5)"      },

	/* Westmere CPUs (32nm): */
	{  6,  5, -1, -1, 37,   2,    -1,    -1, NC, 0             ,     0, "Unknown Core i3/i5"       },
	{  6, 12, -1, -1, 44,  -1,    -1,    -1, WESTMERE, XEON_   ,     0, "Westmere (Xeon)"          },
	{  6, 12, -1, -1, 44,  -1,    -1, 12288, WESTMERE, XEON_   ,     0, "Gulftown (Xeon)"          },
	{  6, 12, -1, -1, 44,   4,    -1, 12288, NC, CORE_|_I_|_7  ,     0, "Gulftown (Core i7)"       },
	{  6,  5, -1, -1, 37,   2,    -1,  4096, NC, CORE_|_I_|_5  ,     0, "Clarkdale (Core i5)"      },
	{  6,  5, -1, -1, 37,   2,    -1,  4096, NC, CORE_|_I_|_3  ,     0, "Clarkdale (Core i3)"      },
	{  6,  5, -1, -1, 37,   2,    -1,    -1, NC, PENTIUM_      ,     0, "Arrandale"                },
	{  6,  5, -1, -1, 37,   2,    -1,  4096, NC, CORE_|_I_|_7  ,     0, "Arrandale (Core i7)"      },
	{  6,  5, -1, -1, 37,   2,    -1,  3072, NC, CORE_|_I_|_5  ,     0, "Arrandale (Core i5)"      },
	{  6,  5, -1, -1, 37,   2,    -1,  3072, NC, CORE_|_I_|_3  ,     0, "Arrandale (Core i3)"      },

	/* Sandy Bridge CPUs (2nd gen, 32nm): */
	{  6, 10, -1, -1, 42,  -1,    -1,    -1, NC, 0             ,     0, "Unknown Sandy Bridge"     },
	{  6, 10, -1, -1, 42,  -1,    -1,    -1, NC, XEON_         ,     0, "Sandy Bridge (Xeon)"      },
	{  6, 10, -1, -1, 42,  -1,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Sandy Bridge (Core i7)"   },
	{  6, 10, -1, -1, 42,   4,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Sandy Bridge (Core i7)"   },
	{  6, 10, -1, -1, 42,   4,    -1,    -1, NC, CORE_|_I_|_5  ,     0, "Sandy Bridge (Core i5)"   },
	{  6, 10, -1, -1, 42,   2,    -1,    -1, NC, CORE_|_I_|_3  ,     0, "Sandy Bridge (Core i3)"   },
	{  6, 10, -1, -1, 42,   2,    -1,    -1, NC, PENTIUM_      ,     0, "Sandy Bridge (Pentium)"   },
	{  6, 10, -1, -1, 42,   1,    -1,    -1, NC, CELERON_      ,     0, "Sandy Bridge (Celeron)"   },
	{  6, 10, -1, -1, 42,   2,    -1,    -1, NC, CELERON_      ,     0, "Sandy Bridge (Celeron)"   },
	{  6, 13, -1, -1, 45,  -1,    -1,    -1, NC, CORE_|_I_|_3  ,     0, "Sandy Bridge-E"           },
	{  6, 13, -1, -1, 45,  -1,    -1,    -1, NC, XEON_         ,     0, "Sandy Bridge-E (Xeon)"    },

	/* Ivy Bridge CPUs (3rd gen, 22nm): */
	{  6, 10, -1, -1, 58,  -1,    -1,    -1, NC, XEON_         ,     0, "Ivy Bridge (Xeon)"        },
	{  6, 10, -1, -1, 58,   4,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Ivy Bridge (Core i7)"     },
	{  6, 10, -1, -1, 58,   4,    -1,    -1, NC, CORE_|_I_|_5  ,     0, "Ivy Bridge (Core i5)"     },
	{  6, 10, -1, -1, 58,   2,    -1,    -1, NC, CORE_|_I_|_3  ,     0, "Ivy Bridge (Core i3)"     },
	{  6, 10, -1, -1, 58,   2,    -1,    -1, NC, PENTIUM_      ,     0, "Ivy Bridge (Pentium)"     },
	{  6, 10, -1, -1, 58,   1,    -1,    -1, NC, CELERON_      ,     0, "Ivy Bridge (Celeron)"     },
	{  6, 10, -1, -1, 58,   2,    -1,    -1, NC, CELERON_      ,     0, "Ivy Bridge (Celeron)"     },
	{  6, 14, -1, -1, 62,  -1,    -1,    -1, NC, XEON_         ,     0, "Ivy Bridge-E (Xeon)"      },
	{  6, 14, -1, -1, 62,  -1,    -1,    -1, NC, 0             ,     0, "Ivy Bridge-E"             },

	/* Haswell CPUs (4th gen, 22nm): */
	{  6, 12, -1, -1, 60,  -1,    -1,    -1, NC, XEON_         ,     0, "Haswell (Xeon)"           },
	{  6, 12, -1, -1, 60,   4,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Haswell (Core i7)"        },
	{  6,  5, -1, -1, 69,   4,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Haswell (Core i7)"        },
	{  6,  6, -1, -1, 70,   4,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Haswell (Core i7)"        },
	{  6, 12, -1, -1, 60,   4,    -1,    -1, NC, CORE_|_I_|_5  ,     0, "Haswell (Core i5)"        },
	{  6,  5, -1, -1, 69,   4,    -1,    -1, NC, CORE_|_I_|_5  ,     0, "Haswell (Core i5)"        },
	{  6, 12, -1, -1, 60,   2,    -1,    -1, NC, CORE_|_I_|_5  ,     0, "Haswell (Core i5)"        },
	{  6,  5, -1, -1, 69,   2,    -1,    -1, NC, CORE_|_I_|_5  ,     0, "Haswell (Core i5)"        },
	{  6, 12, -1, -1, 60,   2,    -1,    -1, NC, CORE_|_I_|_3  ,     0, "Haswell (Core i3)"        },
	{  6,  5, -1, -1, 69,   2,    -1,    -1, NC, CORE_|_I_|_3  ,     0, "Haswell (Core i3)"        },
	{  6, 12, -1, -1, 60,   2,    -1,    -1, NC, PENTIUM_      ,     0, "Haswell (Pentium)"        },
	{  6, 12, -1, -1, 60,   2,    -1,    -1, NC, CELERON_      ,     0, "Haswell (Celeron)"        },
	{  6, 12, -1, -1, 60,   1,    -1,    -1, NC, CELERON_      ,     0, "Haswell (Celeron)"        },
	{  6, 15, -1, -1, 63,  -1,    -1,    -1, NC, 0             ,     0, "Haswell-E"                },

	/* Broadwell CPUs (5th gen, 14nm): */
	{  6,  7, -1, -1, 71,   4,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Broadwell (Core i7)"      },
	{  6,  7, -1, -1, 71,   4,    -1,    -1, NC, CORE_|_I_|_5  ,     0, "Broadwell (Core i5)"      },
	{  6, 13, -1, -1, 61,   4,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Broadwell-U (Core i7)"    },
	{  6, 13, -1, -1, 61,   2,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Broadwell-U (Core i7)"    },
	{  6, 13, -1, -1, 61,   2,    -1,    -1, NC, CORE_|_I_|_5  ,     0, "Broadwell-U (Core i5)"    },
	{  6, 13, -1, -1, 61,   2,    -1,    -1, NC, CORE_|_I_|_3  ,     0, "Broadwell-U (Core i3)"    },
	{  6, 13, -1, -1, 61,   2,    -1,    -1, NC, PENTIUM_      ,     0, "Broadwell-U (Pentium)"    },
	{  6, 13, -1, -1, 61,   2,    -1,    -1, NC, CELERON_      ,     0, "Broadwell-U (Celeron)"    },
	{  6, 13, -1, -1, 61,   2,    -1,    -1, NA, 0             ,     0, "Broadwell-U (Core M)"     },
	{  6, 15, -1, -1, 79,  -1,    -1,    -1, NC, XEON_         ,     0, "Broadwell-E (Xeon)"       },
	{  6, 15, -1, -1, 79,   2,    -1,    -1, NC, CORE_|_I_|_3  ,     0, "Broadwell-E (Core i3)"    },
	{  6, 15, -1, -1, 79,   2,    -1,    -1, NC, CORE_|_I_|_5  ,     0, "Broadwell-E (Core i5)"    },
	{  6, 15, -1, -1, 79,   4,    -1,    -1, NC, CORE_|_I_|_5  ,     0, "Broadwell-E (Core i5)"    },
	{  6, 15, -1, -1, 79,   2,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Broadwell-E (Core i7)"    },
	{  6, 15, -1, -1, 79,   4,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Broadwell-E (Core i7)"    },

	/* Skylake CPUs (6th gen, 14nm): */
	{  6, 14, -1, -1, 94,  -1,    -1,    -1, NC, XEON_         ,     0, "Skylake (Xeon)"           },
	{  6, 14, -1, -1, 94,   4,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Skylake (Core i7)"        },
	{  6, 14, -1, -1, 94,   4,    -1,    -1, NC, CORE_|_I_|_5  ,     0, "Skylake (Core i5)"        },
	{  6, 14, -1, -1, 94,   2,    -1,    -1, NC, CORE_|_I_|_3  ,     0, "Skylake (Core i3)"        },
	{  6, 14, -1, -1, 94,   2,    -1,    -1, NC, PENTIUM_      ,     0, "Skylake (Pentium)"        },
	{  6, 14, -1, -1, 78,   2,    -1,    -1, NC, PENTIUM_      ,     0, "Skylake (Pentium)"        },
	{  6, 14, -1, -1, 94,   2,    -1,    -1, NC, CELERON_      ,     0, "Skylake (Celeron)"        },
	{  6, 14, -1, -1, 78,   2,    -1,    -1, NC, CELERON_      ,     0, "Skylake (Celeron)"        },
	{  6, 14, -1, -1, 78,   2,    -1,    -1, NC, CORE_|_M_|_7  ,     0, "Skylake (Core m7)"        },
	{  6, 14, -1, -1, 78,   2,    -1,    -1, NC, CORE_|_M_|_5  ,     0, "Skylake (Core m5)"        },
	{  6, 14, -1, -1, 78,   2,    -1,    -1, NC, CORE_|_M_|_3  ,     0, "Skylake (Core m3)"        },
	{  6,  5, -1, -1, 85,   8,    -1,    -1, NC, XEON_,              0, "Skylake (Xeon Scalable)"  },
	{  6,  5, -1, -1, 85,  -1,    -1,    -1, NC, CORE_|_I_|_9,       0, "Skylake-X (Core i9)"      }, /* 10 to 18 cores */
	{  6,  5, -1, -1, 85,  -1,    -1,    -1, NC, CORE_|_I_|_7,       0, "Skylake-X (Core i7)"      }, /* 6 to 8 cores */

	/* Kaby Lake CPUs (7th gen, 14nm): */
	{  6, 14, -1, -1, 158,  4,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Kaby Lake (Core i7)"      },
	{  6, 14, -1, -1, 158,  4,    -1,    -1, NC, CORE_|_I_|_5  ,     0, "Kaby Lake (Core i5)"      },
	{  6, 14, -1, -1, 158,  2,    -1,    -1, NC, CORE_|_I_|_3  ,     0, "Kaby Lake (Core i3)"      },
	{  6, 14, -1, -1, 158,  2,    -1,    -1, NC, PENTIUM_      ,     0, "Kaby Lake (Pentium)"      },
	{  6, 14, -1, -1, 158,  2,    -1,    -1, NC, CELERON_      ,     0, "Kaby Lake (Celeron)"      },
	{  6, 14,  9, -1, 142,  2,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Kaby Lake-U (Core i7)"    },
	{  6, 14,  9, -1, 142,  2,    -1,    -1, NC, CORE_|_I_|_5  ,     0, "Kaby Lake-U (Core i5)"    },
	{  6, 14,  9, -1, 142,  2,    -1,    -1, NC, CORE_|_I_|_3  ,     0, "Kaby Lake-U (Core i3)"    },
	{  6, 14,  9, -1, 142,  2,    -1,    -1, NC, PENTIUM_      ,     0, "Kaby Lake-U (Pentium)"    },
	{  6, 14,  9, -1, 142,  2,    -1,    -1, NC, CELERON_      ,     0, "Kaby Lake-U (Celeron)"    },
	{  6, 14,  9, -1, 142,  2,    -1,    -1, NC, CORE_|_M_|_3  ,     0, "Kaby Lake-U (Core m3)"    },

	/* Coffee Lake CPUs (8th gen, 14nm): */
	{  6, 14, 10, -1, 158,  8,    -1,    -1, NC, CORE_|_I_|_9  ,     0, "Coffee Lake (Core i9)"    },
	{  6, 14, 10, -1, 158,  8,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Coffee Lake (Core i7)"    },
	{  6, 14, 10, -1, 158,  6,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Coffee Lake (Core i7)"    },
	{  6, 14, 10, -1, 158,  6,    -1,    -1, NC, CORE_|_I_|_5  ,     0, "Coffee Lake (Core i5)"    },
	{  6, 14, 10, -1, 158,  4,    -1,    -1, NC, CORE_|_I_|_3  ,     0, "Coffee Lake (Core i3)"    },
	{  6, 14, 10, -1, 158,  2,    -1,    -1, NC, PENTIUM_      ,     0, "Coffee Lake (Pentium)"    },
	{  6, 14, 10, -1, 158,  2,    -1,    -1, NC, CELERON_      ,     0, "Coffee Lake (Celeron)"    },
	{  6, 14, 10, -1, 142,  4,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Coffee Lake-U (Core i7)"  },
	{  6, 14, 10, -1, 142,  4,    -1,    -1, NC, CORE_|_I_|_5  ,     0, "Coffee Lake-U (Core i5)"  },
	{  6, 14, 10, -1, 142,  2,    -1,    -1, NC, CORE_|_I_|_3  ,     0, "Coffee Lake-U (Core i3)"  },
	{  6,  6, -1, -1, 102,  2,    -1,    -1, NC, CORE_|_I_|_3  ,     0, "Cannon Lake (Core i3)"    }, /* Core i3 8121U */
	{  6,  6, -1, -1, 102,  2,    -1,    -1, NC, CORE_|_M_|_3  ,     0, "Cannon Lake (Core m3)"    }, /* Core m3 8114Y */

	/* Coffee Lake Refresh CPUs (9th gen, 14nm): */
	{  6, 14, 13, -1, 158,  8,    -1,    -1, NC, CORE_|_I_|_9  ,     0, "Coffee Lake-R (Core i9)"  },
	{  6, 14, 12, -1, 158,  8,    -1,    -1, NC, CORE_|_I_|_9  ,     0, "Coffee Lake-R (Core i9)"  },
	{  6, 14, 13, -1, 158,  8,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Coffee Lake-R (Core i7)"  },
	{  6, 14, 12, -1, 158,  8,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Coffee Lake-R (Core i7)"  },
	{  6, 14, 13, -1, 158,  6,    -1,    -1, NC, CORE_|_I_|_5  ,     0, "Coffee Lake-R (Core i5)"  },
	{  6, 14, 11, -1, 158,  4,    -1,    -1, NC, CORE_|_I_|_3  ,     0, "Coffee Lake-R (Core i3)"  },

	/* Comet Lake CPUs (10th gen, 14nm): */
	{  6,  5, -1, -1, 165, 10,    -1,    -1, NC, CORE_|_I_|_9  ,     0, "Comet Lake (Core i9)"     },
	{  6,  5, -1, -1, 165,  8,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Comet Lake (Core i7)"     },
	{  6,  5, -1, -1, 165,  6,    -1,    -1, NC, CORE_|_I_|_5  ,     0, "Comet Lake (Core i5)"     },
	{  6,  5, -1, -1, 165,  4,    -1,    -1, NC, CORE_|_I_|_3  ,     0, "Comet Lake (Core i3)"     },
	{  6,  5, -1, -1, 165,  2,    -1,    -1, NC, PENTIUM_      ,     0, "Comet Lake (Pentium)"     },
	{  6,  5, -1, -1, 165,  2,    -1,    -1, NC, CELERON_      ,     0, "Comet Lake (Celeron)"     },
	{  6, 14, -1, -1, 126,  4,    -1,    -1, NC, CORE_|_I_|_7  ,     0, "Ice Lake (Core i7)"       },
	{  6, 14, -1, -1, 126,  4,    -1,    -1, NC, CORE_|_I_|_5  ,     0, "Ice Lake (Core i5)"       },
	{  6, 14, -1, -1, 126,  2,    -1,    -1, NC, CORE_|_I_|_3  ,     0, "Ice Lake (Core i3)"       },
	/* F   M   S  EF   EM   C     L2     L3               Brand */


	/* Itaniums */
	{  7, -1, -1, -1, -1,   1,    -1,    -1, NC, 0             ,     0, "Itanium"                  },
	{ 15, -1, -1, 16, -1,   1,    -1,    -1, NC, 0             ,     0, "Itanium 2"                },
};


static void load_intel_features(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	const struct feature_map_t matchtable_edx1[] = {
		{ 18, CPU_FEATURE_PN },
		{ 21, CPU_FEATURE_DTS },
		{ 22, CPU_FEATURE_ACPI },
		{ 27, CPU_FEATURE_SS },
		{ 29, CPU_FEATURE_TM },
		{ 30, CPU_FEATURE_IA64 },
		{ 31, CPU_FEATURE_PBE },
	};
	const struct feature_map_t matchtable_ecx1[] = {
		{  2, CPU_FEATURE_DTS64 },
		{  4, CPU_FEATURE_DS_CPL },
		{  5, CPU_FEATURE_VMX },
		{  6, CPU_FEATURE_SMX },
		{  7, CPU_FEATURE_EST },
		{  8, CPU_FEATURE_TM2 },
		{ 10, CPU_FEATURE_CID },
		{ 14, CPU_FEATURE_XTPR },
		{ 15, CPU_FEATURE_PDCM },
		{ 18, CPU_FEATURE_DCA },
		{ 21, CPU_FEATURE_X2APIC },
	};
	const struct feature_map_t matchtable_edx81[] = {
		{ 20, CPU_FEATURE_XD },
	};
	const struct feature_map_t matchtable_ebx7[] = {
		{  2, CPU_FEATURE_SGX },
		{  4, CPU_FEATURE_HLE },
		{ 11, CPU_FEATURE_RTM },
		{ 16, CPU_FEATURE_AVX512F },
		{ 17, CPU_FEATURE_AVX512DQ },
		{ 18, CPU_FEATURE_RDSEED },
		{ 19, CPU_FEATURE_ADX },
		{ 26, CPU_FEATURE_AVX512PF },
		{ 27, CPU_FEATURE_AVX512ER },
		{ 28, CPU_FEATURE_AVX512CD },
		{ 29, CPU_FEATURE_SHA_NI },
		{ 30, CPU_FEATURE_AVX512BW },
		{ 31, CPU_FEATURE_AVX512VL },
	};
	const struct feature_map_t matchtable_ecx7[] = {
		{ 11, CPU_FEATURE_AVX512VNNI },
	};
	if (raw->basic_cpuid[0][EAX] >= 1) {
		match_features(matchtable_edx1, COUNT_OF(matchtable_edx1), raw->basic_cpuid[1][EDX], data);
		match_features(matchtable_ecx1, COUNT_OF(matchtable_ecx1), raw->basic_cpuid[1][ECX], data);
	}
	if (raw->ext_cpuid[0][EAX] >= 1) {
		match_features(matchtable_edx81, COUNT_OF(matchtable_edx81), raw->ext_cpuid[1][EDX], data);
	}
	// detect TSX/AVX512:
	if (raw->basic_cpuid[0][EAX] >= 7) {
		match_features(matchtable_ebx7, COUNT_OF(matchtable_ebx7), raw->basic_cpuid[7][EBX], data);
		match_features(matchtable_ecx7, COUNT_OF(matchtable_ecx7), raw->basic_cpuid[7][ECX], data);
	}
}

enum _cache_type_t {
	L1I,
	L1D,
	L2,
	L3,
	L4
};
typedef enum _cache_type_t cache_type_t;

static void check_case(uint8_t on, cache_type_t cache, int size, int assoc, int linesize, struct cpu_id_t* data)
{
	if (!on) return;
	switch (cache) {
		case L1I:
			data->l1_instruction_cache = size;
			data->l1_instruction_assoc = assoc;
			data->l1_instruction_cacheline = linesize;
			break;
		case L1D:
			data->l1_data_cache = size;
			data->l1_data_assoc = assoc;
			data->l1_data_cacheline = linesize;
			break;
		case L2:
			data->l2_cache = size;
			data->l2_assoc = assoc;
			data->l2_cacheline = linesize;
			break;
		case L3:
			data->l3_cache = size;
			data->l3_assoc = assoc;
			data->l3_cacheline = linesize;
			break;
		case L4:
			data->l4_cache = size;
			data->l4_assoc = assoc;
			data->l4_cacheline = linesize;
			break;
		default:
			break;
	}
}

static void decode_intel_oldstyle_cache_info(struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	uint8_t f[256] = {0};
	int reg, off;
	uint32_t x;
	for (reg = 0; reg < 4; reg++) {
		x = raw->basic_cpuid[2][reg];
		if (x & 0x80000000) continue;
		for (off = 0; off < 4; off++) {
			f[x & 0xff] = 1;
			x >>= 8;
		}
	}

	check_case(f[0x06], L1I,      8,  4,  32, data);
	check_case(f[0x08], L1I,     16,  4,  32, data);
	check_case(f[0x0A], L1D,      8,  2,  32, data);
	check_case(f[0x0C], L1D,     16,  4,  32, data);
	check_case(f[0x22],  L3,    512,  4,  64, data);
	check_case(f[0x23],  L3,   1024,  8,  64, data);
	check_case(f[0x25],  L3,   2048,  8,  64, data);
	check_case(f[0x29],  L3,   4096,  8,  64, data);
	check_case(f[0x2C], L1D,     32,  8,  64, data);
	check_case(f[0x30], L1I,     32,  8,  64, data);
	check_case(f[0x39],  L2,    128,  4,  64, data);
	check_case(f[0x3A],  L2,    192,  6,  64, data);
	check_case(f[0x3B],  L2,    128,  2,  64, data);
	check_case(f[0x3C],  L2,    256,  4,  64, data);
	check_case(f[0x3D],  L2,    384,  6,  64, data);
	check_case(f[0x3E],  L2,    512,  4,  64, data);
	check_case(f[0x41],  L2,    128,  4,  32, data);
	check_case(f[0x42],  L2,    256,  4,  32, data);
	check_case(f[0x43],  L2,    512,  4,  32, data);
	check_case(f[0x44],  L2,   1024,  4,  32, data);
	check_case(f[0x45],  L2,   2048,  4,  32, data);
	check_case(f[0x46],  L3,   4096,  4,  64, data);
	check_case(f[0x47],  L3,   8192,  8,  64, data);
	check_case(f[0x4A],  L3,   6144, 12,  64, data);
	check_case(f[0x4B],  L3,   8192, 16,  64, data);
	check_case(f[0x4C],  L3,  12288, 12,  64, data);
	check_case(f[0x4D],  L3,  16384, 16,  64, data);
	check_case(f[0x4E],  L2,   6144, 24,  64, data);
	check_case(f[0x60], L1D,     16,  8,  64, data);
	check_case(f[0x66], L1D,      8,  4,  64, data);
	check_case(f[0x67], L1D,     16,  4,  64, data);
	check_case(f[0x68], L1D,     32,  4,  64, data);
	/* The following four entries are trace cache. Intel does not
	 * specify a cache-line size, so we use -1 instead
	 */
	check_case(f[0x70], L1I,     12,  8,  -1, data);
	check_case(f[0x71], L1I,     16,  8,  -1, data);
	check_case(f[0x72], L1I,     32,  8,  -1, data);
	check_case(f[0x73], L1I,     64,  8,  -1, data);

	check_case(f[0x78],  L2,   1024,  4,  64, data);
	check_case(f[0x79],  L2,    128,  8,  64, data);
	check_case(f[0x7A],  L2,    256,  8,  64, data);
	check_case(f[0x7B],  L2,    512,  8,  64, data);
	check_case(f[0x7C],  L2,   1024,  8,  64, data);
	check_case(f[0x7D],  L2,   2048,  8,  64, data);
	check_case(f[0x7F],  L2,    512,  2,  64, data);
	check_case(f[0x82],  L2,    256,  8,  32, data);
	check_case(f[0x83],  L2,    512,  8,  32, data);
	check_case(f[0x84],  L2,   1024,  8,  32, data);
	check_case(f[0x85],  L2,   2048,  8,  32, data);
	check_case(f[0x86],  L2,    512,  4,  64, data);
	check_case(f[0x87],  L2,   1024,  8,  64, data);

	if (f[0x49]) {
		/* This flag is overloaded with two meanings. On Xeon MP
		 * (family 0xf, model 0x6) this means L3 cache. On all other
		 * CPUs (notably Conroe et al), this is L2 cache. In both cases
		 * it means 4MB, 16-way associative, 64-byte line size.
		 */
		if (data->family == 0xf && data->model == 0x6) {
			data->l3_cache = 4096;
			data->l3_assoc = 16;
			data->l3_cacheline = 64;
		} else {
			data->l2_cache = 4096;
			data->l2_assoc = 16;
			data->l2_cacheline = 64;
		}
	}
	if (f[0x40]) {
		/* Again, a special flag. It means:
		 * 1) If no L2 is specified, then CPU is w/o L2 (0 KB)
		 * 2) If L2 is specified by other flags, then, CPU is w/o L3.
		 */
		if (data->l2_cache == -1) {
			data->l2_cache = 0;
		} else {
			data->l3_cache = 0;
		}
	}
}

static void decode_intel_deterministic_cache_info(struct cpu_raw_data_t* raw,
                                                  struct cpu_id_t* data)
{
	int ecx;
	int ways, partitions, linesize, sets, size, level, typenumber;
	cache_type_t type;
	for (ecx = 0; ecx < MAX_INTELFN4_LEVEL; ecx++) {
		typenumber = raw->intel_fn4[ecx][EAX] & 0x1f;
		if (typenumber == 0) break;
		level = (raw->intel_fn4[ecx][EAX] >> 5) & 0x7;
		if (level == 1 && typenumber == 1)
			type = L1D;
		else if (level == 1 && typenumber == 2)
			type = L1I;
		else if (level == 2 && typenumber == 3)
			type = L2;
		else if (level == 3 && typenumber == 3)
			type = L3;
		else if (level == 4 && typenumber == 3)
			type = L4;
		else {
			warnf("deterministic_cache: unknown level/typenumber combo (%d/%d), cannot\n", level, typenumber);
			warnf("deterministic_cache: recognize cache type\n");
			continue;
		}
		ways = ((raw->intel_fn4[ecx][EBX] >> 22) & 0x3ff) + 1;
		partitions = ((raw->intel_fn4[ecx][EBX] >> 12) & 0x3ff) + 1;
		linesize = (raw->intel_fn4[ecx][EBX] & 0xfff) + 1;
		sets = raw->intel_fn4[ecx][ECX] + 1;
		size = ways * partitions * linesize * sets / 1024;
		check_case(1, type, size, ways, linesize, data);
	}
}

static int decode_intel_extended_topology(struct cpu_raw_data_t* raw,
                                           struct cpu_id_t* data)
{
	int i, level_type, num_smt = -1, num_core = -1;
	for (i = 0; i < MAX_INTELFN11_LEVEL; i++) {
		level_type = (raw->intel_fn11[i][ECX] & 0xff00) >> 8;
		switch (level_type) {
			case 0x01:
				num_smt = raw->intel_fn11[i][EBX] & 0xffff;
				break;
			case 0x02:
				num_core = raw->intel_fn11[i][EBX] & 0xffff;
				break;
			default:
				break;
		}
	}
	if (num_smt == -1 || num_core == -1) return 0;
	data->num_logical_cpus = num_core;
	data->num_cores = num_core / num_smt;
	// make sure num_cores is at least 1. In VMs, the CPUID instruction
	// is rigged and may give nonsensical results, but we should at least
	// avoid outputs like data->num_cores == 0.
	if (data->num_cores <= 0) data->num_cores = 1;
	return 1;
}

static void decode_intel_number_of_cores(struct cpu_raw_data_t* raw,
                                         struct cpu_id_t* data)
{
	int logical_cpus = -1, num_cores = -1;

	if (raw->basic_cpuid[0][EAX] >= 11) {
		if (decode_intel_extended_topology(raw, data)) return;
	}

	if (raw->basic_cpuid[0][EAX] >= 1) {
		logical_cpus = (raw->basic_cpuid[1][EBX] >> 16) & 0xff;
		if (raw->basic_cpuid[0][EAX] >= 4) {
			num_cores = 1 + ((raw->basic_cpuid[4][EAX] >> 26) & 0x3f);
		}
	}
	if (data->flags[CPU_FEATURE_HT]) {
		if (num_cores > 1) {
			data->num_cores = num_cores;
			data->num_logical_cpus = logical_cpus;
		} else {
			data->num_cores = 1;
			data->num_logical_cpus = (logical_cpus >= 1 ? logical_cpus : 1);
			if (data->num_logical_cpus == 1)
				data->flags[CPU_FEATURE_HT] = 0;
		}
	} else {
		data->num_cores = data->num_logical_cpus = 1;
	}
}

static intel_code_and_bits_t get_brand_code_and_bits(struct cpu_id_t* data)
{
	intel_code_t code = (intel_code_t) NC;
	intel_code_and_bits_t result;
	uint64_t bits = 0;
	int i = 0;
	const char* bs = data->brand_str;
	const char* s;
	const struct { intel_code_t c; const char *search; } matchtable[] = {
		{ PENTIUM_M, "Pentium(R) M" },
		{ CORE_SOLO, "Pentium(R) Dual  CPU" },
		{ CORE_SOLO, "Pentium(R) Dual-Core" },
		{ PENTIUM_D, "Pentium(R) D" },
		{ CORE_SOLO, "Genuine Intel(R) CPU" },
		{ CORE_SOLO, "Intel(R) Core(TM)" },
		{ DIAMONDVILLE, "CPU [N ][23]## " },
		{ SILVERTHORNE, "CPU Z" },
		{ PINEVIEW, "CPU [ND][45]## " },
		{ CEDARVIEW, "CPU [ND]#### " },
	};

	const struct { uint64_t bit; const char* search; } bit_matchtable[] = {
		{ XEON_, "Xeon" },
		{ _MP_, " MP" },
		{ ATOM_, "Atom(TM) CPU" },
		{ MOBILE_, "Mobile" },
		{ CELERON_, "Celeron" },
		{ PENTIUM_, "Pentium" },
	};

	for (i = 0; i < COUNT_OF(bit_matchtable); i++) {
		if (match_pattern(bs, bit_matchtable[i].search))
			bits |= bit_matchtable[i].bit;
	}

	if ((i = match_pattern(bs, "Core(TM) [im][3579]")) != 0) {
		bits |= CORE_;
		i--;
		switch (bs[i + 9]) {
			case 'i': bits |= _I_; break;
			case 'm': bits |= _M_; break;
		}
		switch (bs[i + 10]) {
			case '3': bits |= _3; break;
			case '5': bits |= _5; break;
			case '7': bits |= _7; break;
			case '9': bits |= _9; break;
		}
	}
	for (i = 0; i < COUNT_OF(matchtable); i++)
		if (match_pattern(bs, matchtable[i].search)) {
			code = matchtable[i].c;
			break;
		}
	debugf(2, "intel matchtable result is %d\n", code);
	if (bits & XEON_) {
		if (match_pattern(bs, "W35##") || match_pattern(bs, "[ELXW]75##"))
			bits |= _7;
		else if (match_pattern(bs, "[ELXW]55##"))
			code = GAINESTOWN;
		else if (match_pattern(bs, "[ELXW]56##"))
			code = WESTMERE;
		else if (data->l3_cache > 0 && data->family == 16)
			/* restrict by family, since later Xeons also have L3 ... */
			code = IRWIN;
	}
	if (match_all(bits, XEON_ + _MP_) && data->l3_cache > 0)
		code = POTOMAC;
	if (code == CORE_SOLO) {
		s = strstr(bs, "CPU");
		if (s) {
			s += 3;
			while (*s == ' ') s++;
			if (*s == 'T')
				bits |= MOBILE_;
		}
	}
	if (code == CORE_SOLO) {
		switch (data->num_cores) {
			case 1: break;
			case 2:
			{
				code = CORE_DUO;
				if (data->num_logical_cpus > 2)
					code = DUAL_CORE_HT;
				break;
			}
			case 4:
			{
				code = QUAD_CORE;
				if (data->num_logical_cpus > 4)
					code = QUAD_CORE_HT;
				break;
			}
			default:
				code = MORE_THAN_QUADCORE; break;
		}
	}

	if (code == CORE_DUO && (bits & MOBILE_) && data->model != 14) {
		if (data->ext_model < 23) {
			code = MEROM;
		} else {
			code = PENRYN;
		}
	}
	if (data->ext_model == 23 &&
		(code == CORE_DUO || code == PENTIUM_D || (bits & CELERON_))) {
		code = WOLFDALE;
	}

	result.code = code;
	result.bits = bits;
	return result;
}

static intel_model_t get_model_code(struct cpu_id_t* data)
{
	int i = 0;
	int l = (int) strlen(data->brand_str);
	const char *bs = data->brand_str;
	int mod_flags = 0, model_no = 0, ndigs = 0;
	/* If the CPU is a Core ix, then just return the model number generation: */
	if ((i = match_pattern(bs, "Core(TM) i[357]")) != 0) {
		i += 11;
		if (i + 4 >= l) return UNKNOWN;
		if (bs[i] == '2') return _2xxx;
		if (bs[i] == '3') return _3xxx;
		return UNKNOWN;
	}

	/* For Core2-based Xeons: */
	while (i < l - 3) {
		if (bs[i] == 'C' && bs[i+1] == 'P' && bs[i+2] == 'U')
			break;
		i++;
	}
	if (i >= l - 3) return UNKNOWN;
	i += 3;
	while (i < l - 4 && bs[i] == ' ') i++;
	if (i >= l - 4) return UNKNOWN;
	while (i < l - 4 && !isdigit(bs[i])) {
		if (bs[i] >= 'A' && bs[i] <= 'Z')
			mod_flags |= (1 << (bs[i] - 'A'));
		i++;
	}
	if (i >= l - 4) return UNKNOWN;
	while (isdigit(bs[i])) {
		ndigs++;
		model_no = model_no * 10 + (int) (bs[i] - '0');
		i++;
	}
	if (ndigs != 4) return UNKNOWN;
#define HAVE(ch, flags) ((flags & (1 << ((int)(ch-'A')))) != 0)
	switch (model_no / 100) {
		case 30: return _3000;
		case 31: return _3100;
		case 32:
		{
			return (HAVE('X', mod_flags)) ? X3200 : _3200;
		}
		case 33:
		{
			return (HAVE('X', mod_flags)) ? X3300 : _3300;
		}
		case 51: return _5100;
		case 52: return _5200;
		case 53: return _5300;
		case 54: return _5400;
		default:
			return UNKNOWN;
	}
#undef HAVE
}

static void decode_intel_sgx_features(const struct cpu_raw_data_t* raw, struct cpu_id_t* data)
{
	struct cpu_epc_t epc;
	int i;

	if (raw->basic_cpuid[0][EAX] < 0x12) return; // no 12h leaf
	if (raw->basic_cpuid[0x12][EAX] == 0) return; // no sub-leafs available, probably it's disabled by BIOS

	// decode sub-leaf 0:
	if (raw->basic_cpuid[0x12][EAX] & 1) data->sgx.flags[INTEL_SGX1] = 1;
	if (raw->basic_cpuid[0x12][EAX] & 2) data->sgx.flags[INTEL_SGX2] = 1;
	if (data->sgx.flags[INTEL_SGX1] || data->sgx.flags[INTEL_SGX2])
		data->sgx.present = 1;
	data->sgx.misc_select = raw->basic_cpuid[0x12][EBX];
	data->sgx.max_enclave_32bit = (raw->basic_cpuid[0x12][EDX]     ) & 0xff;
	data->sgx.max_enclave_64bit = (raw->basic_cpuid[0x12][EDX] >> 8) & 0xff;

	// decode sub-leaf 1:
	data->sgx.secs_attributes = raw->intel_fn12h[1][EAX] | (((uint64_t) raw->intel_fn12h[1][EBX]) << 32);
	data->sgx.secs_xfrm       = raw->intel_fn12h[1][ECX] | (((uint64_t) raw->intel_fn12h[1][EDX]) << 32);

	// decode higher-order subleafs, whenever present:
	data->sgx.num_epc_sections = -1;
	for (i = 0; i < 1000000; i++) {
		epc = cpuid_get_epc(i, raw);
		if (epc.length == 0) {
			debugf(2, "SGX: epc section request for %d returned null, no more EPC sections.\n", i);
			data->sgx.num_epc_sections = i;
			break;
		}
	}
	if (data->sgx.num_epc_sections == -1) {
		debugf(1, "SGX: warning: seems to be infinitude of EPC sections.\n");
		data->sgx.num_epc_sections = 1000000;
	}
}

struct cpu_epc_t cpuid_get_epc(int index, const struct cpu_raw_data_t* raw)
{
	uint32_t regs[4];
	struct cpu_epc_t retval = {0, 0};
	if (raw && index < MAX_INTELFN12H_LEVEL - 2) {
		// this was queried already, use the data:
		memcpy(regs, raw->intel_fn12h[2 + index], sizeof(regs));
	} else {
		// query this ourselves:
		regs[0] = 0x12;
		regs[2] = 2 + index;
		regs[1] = regs[3] = 0;
		cpu_exec_cpuid_ext(regs);
	}

	// decode values:
	if ((regs[0] & 0xf) == 0x1) {
		retval.start_addr |= (regs[0] & 0xfffff000); // bits [12, 32) -> bits [12, 32)
		retval.start_addr |= ((uint64_t) (regs[1] & 0x000fffff)) << 32; // bits [0, 20) -> bits [32, 52)
		retval.length     |= (regs[2] & 0xfffff000); // bits [12, 32) -> bits [12, 32)
		retval.length     |= ((uint64_t) (regs[3] & 0x000fffff)) << 32; // bits [0, 20) -> bits [32, 52)
	}
	return retval;
}

int cpuid_identify_intel(struct cpu_raw_data_t* raw, struct cpu_id_t* data, struct internal_id_info_t* internal)
{
	intel_code_and_bits_t brand;
	intel_model_t model_code;
	int i;
	char* brand_code_str = NULL;

	load_intel_features(raw, data);
	if (raw->basic_cpuid[0][EAX] >= 4) {
		/* Deterministic way is preferred, being more generic */
		decode_intel_deterministic_cache_info(raw, data);
	} else if (raw->basic_cpuid[0][EAX] >= 2) {
		decode_intel_oldstyle_cache_info(raw, data);
	}
	decode_intel_number_of_cores(raw, data);

	brand = get_brand_code_and_bits(data);
	model_code = get_model_code(data);
	for (i = 0; i < COUNT_OF(intel_bcode_str); i++) {
		if (brand.code == intel_bcode_str[i].code) {
			brand_code_str = intel_bcode_str[i].str;
			break;
		}
	}
	if (brand_code_str)
		debugf(2, "Detected Intel brand code: %d (%s)\n", brand.code, brand_code_str);
	else
		debugf(2, "Detected Intel brand code: %d\n", brand.code);
	if (brand.bits) {
		debugf(2, "Detected Intel bits: ");
		debug_print_lbits(2, brand.bits);
	}
	debugf(2, "Detected Intel model code: %d\n", model_code);

	internal->code.intel = brand.code;
	internal->bits = brand.bits;

	if (data->flags[CPU_FEATURE_SGX]) {
		debugf(2, "SGX seems to be present, decoding...\n");
		// if SGX is indicated by the CPU, verify its presence:
		decode_intel_sgx_features(raw, data);
	}

	internal->score = match_cpu_codename(cpudb_intel, COUNT_OF(cpudb_intel), data,
		brand.code, brand.bits, model_code);
	return 0;
}

void cpuid_get_list_intel(struct cpu_list_t* list)
{
	generic_get_cpu_list(cpudb_intel, COUNT_OF(cpudb_intel), list);
}
