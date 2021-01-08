set(COMPONENT_SRCS "main.c" "display.c" "network.c")
set(COMPONENT_ADD_INCLUDEDIRS "." "./includes")

register_component()

target_add_binary_data(${COMPONENT_TARGET} "cert/aws-root-ca.pem" TEXT)