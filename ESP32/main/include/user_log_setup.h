#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuration des niveaux de log au démarrage.
 * À appeler très tôt dans app_main().
 */
void user_log_setup(void);

#ifdef __cplusplus
}
#endif
