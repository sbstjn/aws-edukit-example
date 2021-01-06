set(COMPONENT_SRCS "main.c" "display.c" "network.c")
set(COMPONENT_ADD_INCLUDEDIRS "." "./includes")

register_component()

if(CONFIG_AWS_IOT_USE_HARDWARE_SECURE_ELEMENT)
    target_add_binary_data(${COMPONENT_TARGET} "cert/aws-root-ca.pem" TEXT)
endif()