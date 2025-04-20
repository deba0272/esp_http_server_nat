idf_component_register(SRCS "http_nat_demo.c"
                    INCLUDE_DIRS ".")
target_add_binary_data(${COMPONENT_TARGET} "server_cert.pem" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "server_key.pem" TEXT)

