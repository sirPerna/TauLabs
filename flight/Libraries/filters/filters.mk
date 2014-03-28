#
# Rules to add the filter architecture
#

FILTERLIBINC = $(FILTERLIB)
EXTRAINCDIRS += $(FILTERLIBINC)

SRC += $(FILTERLIB)/cfnav_interface.c
SRC += $(FILTERLIB)/cf_interface.c
SRC += $(FILTERLIB)/filter_infrastructure_se3.c
SRC += $(FILTERLIB)/filter_interface.c

# Include SREKF from Korken
SRC += $(FILTERLIB)/srekf_interface.c
SRC += $(FILTERLIB)/srekf/attitude_ekf.c
SRC += $(FILTERLIB)/srekf/quaternion.c
SRC += $(FILTERLIB)/srekf/trigonometry.c
SRC += $(FILTERLIB)/srekf/linear_algebra.c
EXTRAINCDIRS += $(FILTERLIB)/srekf