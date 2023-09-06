# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS
    "ads7138/ads7138.cc"
    "as5055/as5055.cc"
    "esc/esc.cc"
    "motors/motors.cc"
    "mpu6500/mpu6500.cc"
    "main.cc"
)
set(COMPONENT_ADD_INCLUDEDIRS "")

register_component()
