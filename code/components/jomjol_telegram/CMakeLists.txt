idf_component_register(SRCS "interface_telegram.cpp"
                       INCLUDE_DIRS "."
                       REQUIRES esp_http_client jomjol_logfile json)