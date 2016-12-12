/*
 * g711.h
 *
 *  Created on: 2012-6-20
 *      Author: dvr
 */

#ifndef G711_H_
#define G711_H_

int G711AlawDecoder(char indata[], short outdata[], int len);

int G711UlawDecoder(char indata[], short outdata[], int len);

#endif /* G711_H_ */
