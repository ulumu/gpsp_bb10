/*
 * bbHelper.h
 *
 *  Created on: Jan 8, 2015
 *      Author: ULUMU
 */

#ifndef BBHELPER_H_
#define BBHELPER_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Helper function interface with C program
 */
void bbShowAlert(const char *title, const char *content);
void bbShowNotification(const char *content);
void loadRomDialog(char *filename);
void loadConfiguration(void);

#ifdef __cplusplus
}
#endif


#endif /* BBHELPER_H_ */
