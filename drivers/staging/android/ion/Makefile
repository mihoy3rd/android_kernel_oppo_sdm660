obj-$(CONFIG_ION) +=	ion.o ion_heap.o ion_page_pool.o ion_system_heap.o \
			ion_carveout_heap.o ion_chunk_heap.o
ifdef CONFIG_ION_MSM
obj-$(CONFIG_CMA) += ion_cma_heap.o ion_cma_secure_heap.o
endif
obj-$(CONFIG_ION_TEST) += ion_test.o
ifdef CONFIG_COMPAT
obj-$(CONFIG_ION) += compat_ion.o
endif
obj-$(CONFIG_ION_DUMMY) += ion_dummy_driver.o
obj-$(CONFIG_ION_TEGRA) += tegra/
obj-$(CONFIG_ION_MSM) += msm/
obj-$(CONFIG_ION_MSM) += ion_system_secure_heap.o

# TODO: remove me b/62057517
subdir-ccflags-y += \
	-Wno-constant-logical-operand