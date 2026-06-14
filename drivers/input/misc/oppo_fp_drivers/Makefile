# VENDOR_EDIT
# OPPO fingerprint drivers top-level Makefile

ifeq ($(CONFIG_OPPO_FINGERPRINT),y)
obj-y        += oppo_fp_common/
obj-y        += fpc/

# MACH_OPPO_PBCM10 (R15x/K1, 18005): silead + goodix_optical_fp
ifneq ($(or $(CONFIG_MACH_OPPO_PBCM10),),)
obj-y        += goodix_optical_fp/
obj-y        += silead/
endif

# MACH_OPPO_R11S (17011/17015) or MACH_OPPO_PAAM00 (17081/17085): goodix_fp
ifneq ($(or $(CONFIG_MACH_OPPO_R11S),$(CONFIG_MACH_OPPO_PAAM00)),)
obj-y        += goodix_fp/
endif

endif
