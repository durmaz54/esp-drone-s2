idf_component_register(SRCS "BNO08x.cpp" "main.cpp" "motor.cpp" "dz_pid.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES driver esp_timer
                    PRIV_REQUIRES esp_event esp_wifi nvs_flash
                    )
