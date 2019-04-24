#include <jni.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <winsock2.h>
#include <io.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#endif
#include "nl_mansoft_tlspoolsocket_TlspoolSocket.h"   // Generated
#ifdef TEST
#ifdef _WIN32
#define FILENAME_CRYPT "c:\\tmp\\crypt.txt"
#define FILENAME_PLAIN "c:\\tmp\\plain.txt"
#else /* _WIN32 */
#define FILENAME_CRYPT "/tmp/crypt.txt"
#define FILENAME_PLAIN "/tmp/plain.txt"
#endif /* _WIN32 */
#else /* TEST */
#include <tlspool/starttls.h>
#endif /* TEST */

#ifdef __cplusplus
extern "C" {
#endif

int ipproto_to_sockettype(uint8_t ipproto);
#ifdef WINDOWS_PORT
int dumb_socketpair(SOCKET socks[2], int sock_type, int make_overlapped);
int convert_socket_to_posix(SOCKET s, bool autoclose);
#endif

/*
 * Class:     TlspoolSocket
 * Method:    startTls0
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_nl_mansoft_tlspoolsocket_TlspoolSocket_startTls0
	(JNIEnv *env, jobject thisObj, jint flags, jint local, jint ipproto, jint streamid, jstring localid, jstring remoteid, jstring service, jint timeout)
{
	int rc = 0;
	// Get a reference to this object's class
	jclass thisClass = env->GetObjectClass(thisObj);
#ifdef TEST
	int cryptfd = open(FILENAME_CRYPT, O_CREAT | O_RDWR, S_IREAD | S_IWRITE);
	// Get the Field ID of the instance variable "cryptfd"
	jfieldID fidCryptfd = env->GetFieldID(thisClass, "cryptfd", "I");
	if (NULL == fidCryptfd) return -1;
	// Get the int given the Field ID
	env->SetIntField(thisObj, fidCryptfd, cryptfd);

	int plainfd = open(FILENAME_PLAIN, O_CREAT | O_RDWR, S_IREAD | S_IWRITE);
	jfieldID fidPlainfd = env->GetFieldID(thisClass, "plainfd", "I");
	if (NULL == fidPlainfd) return -1;
	// Get the int given the Field ID
	env->SetIntField(thisObj, fidPlainfd, plainfd);
#else /* TEST */
	int plainfd = -1;
	int type = ipproto_to_sockettype (ipproto);
	int soxx [2];
#ifndef WINDOWS_PORT
	rc = socketpair (AF_UNIX, type, 0, soxx);
#else /* WINDOWS_PORT */
	SOCKET sockets [2];
	rc = dumb_socketpair(sockets, type, 1);
	soxx[0] = convert_socket_to_posix(sockets[0], true);
	soxx[1] = convert_socket_to_posix(sockets[1], true);
#endif /* WINDOWS_PORT */
	if (rc == 0) {
//fprintf(stderr, "soxx[0] = %d, soxx[1] = %d\n", soxx[0], soxx[1]);
		// Get the Field ID of the instance variable "cryptfd"
		jfieldID fidCryptfd = env->GetFieldID(thisClass, "cryptfd", "I");
		if (NULL == fidCryptfd) return -1;
//fprintf(stderr, "fidCryptfd = %d\n", fidCryptfd);
		// Get the int given the Field ID
		env->SetIntField(thisObj, fidCryptfd, soxx[0]);
		const char *localidCStr = env->GetStringUTFChars(localid, NULL);
		if (NULL == localidCStr) return -1;
		const char *remoteidCStr = env->GetStringUTFChars(remoteid, NULL);
		if (NULL == remoteidCStr) return -1;
		const char *serviceCStr = env->GetStringUTFChars(service, NULL);
		if (NULL == serviceCStr) return -1;
		starttls_t tlsdata = {
			flags,
			local,
			ipproto,
			streamid,
		};
		strcpy(tlsdata.localid, localidCStr);
		strcpy(tlsdata.remoteid, remoteidCStr);
		strcpy((char *) tlsdata.service, serviceCStr);
		tlsdata.timeout = timeout;
fprintf(stderr, "tlspool_starttls: flags = %d, localid = %d, ipproto = %d, streamid = %d, localid = %s, remoteid = %s, service = %s, timeout = %d\n",
			tlsdata.flags,
			tlsdata.local,
			tlsdata.ipproto,
			tlsdata.streamid,
			tlsdata.localid,
			tlsdata.remoteid,
			(char *) tlsdata.service,
			timeout
		);
		rc = tlspool_starttls (soxx[1], &tlsdata, &plainfd, NULL);
fprintf(stderr, "tlspool_starttls: rc = %d\n", rc);
			env->ReleaseStringUTFChars(localid, localidCStr);
			env->ReleaseStringUTFChars(remoteid, remoteidCStr);
			env->ReleaseStringUTFChars(service, serviceCStr);
		if (rc == 0) {
			jfieldID fidPlainfd = env->GetFieldID(thisClass, "plainfd", "I");
			if (NULL == fidPlainfd) return -1;
//fprintf(stderr, "fidPlainfd = %d\n", fidPlainfd);
			// Get the int given the Field ID
			env->SetIntField(thisObj, fidPlainfd, plainfd);				
		} else {
			perror ("Failed to STARTTLS on testcli");
			if (plainfd >= 0) {
				close (plainfd);
			}
		}
	}
#endif /* TEST */
	return rc;
}

/*
 * Class:     TlspoolSocket
 * Method:    readEncrypted
 * Signature: ([BII)I
 */
JNIEXPORT jint JNICALL Java_nl_mansoft_tlspoolsocket_TlspoolSocket_readEncrypted
(JNIEnv *env, jobject thisObj, jbyteArray inJNIArray, jint off, jint len)
{
	jbyte *inCArray = env->GetByteArrayElements(inJNIArray, NULL);
	if (NULL == inCArray) return 0;
	jsize length = env->GetArrayLength(inJNIArray);
	// Get a reference to this object's class
	jclass thisClass = env->GetObjectClass(thisObj);

	// Get the Field ID of the instance variable "cryptfd"
	jfieldID fidCryptfd = env->GetFieldID(thisClass, "cryptfd", "I");
	if (NULL == fidCryptfd) return 0;
	// Get the int given the Field ID
	int cryptfd = env->GetIntField(thisObj, fidCryptfd);
//fprintf(stderr, "readEncrypted: cryptfd = %d\n", cryptfd);
	int bytesread = recv(cryptfd, (char*) inCArray + off, len, 0);
//fprintf(stderr, "readEncrypted: bytesread = %d\n", bytesread);
	env->ReleaseByteArrayElements(inJNIArray, inCArray, 0); // release resources
	return bytesread;
}
/*
 * Class:     TlspoolSocket
 * Method:    writeEncrypted
 * Signature: ([BII)I
 */
JNIEXPORT jint JNICALL Java_nl_mansoft_tlspoolsocket_TlspoolSocket_writeEncrypted
(JNIEnv *env, jobject thisObj, jbyteArray inJNIArray, jint off, jint len)
{
	jbyte *inCArray = env->GetByteArrayElements(inJNIArray, NULL);
	if (NULL == inCArray) return 0;
	jsize length = env->GetArrayLength(inJNIArray);
	// Get a reference to this object's class
	jclass thisClass = env->GetObjectClass(thisObj);

	// Get the Field ID of the instance variable "cryptfd"
	jfieldID fidCryptfd = env->GetFieldID(thisClass, "cryptfd", "I");
	if (NULL == fidCryptfd) return 0;
	// Get the int given the Field ID
	int cryptfd = env->GetIntField(thisObj, fidCryptfd);
//fprintf(stderr, "writeEncrypted: cryptfd = %d\n", cryptfd);
	int byteswritten = send(cryptfd, (char*) inCArray + off, len, 0);
	env->ReleaseByteArrayElements(inJNIArray, inCArray, 0); // release resources
//fprintf(stderr, "writeEncrypted: byteswritten = %d\n", byteswritten);
	return byteswritten;
}
/*
 * Class:     nl_mansoft_tlspoolsocket_TlspoolSocket
 * Method:    stopTls0
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_nl_mansoft_tlspoolsocket_TlspoolSocket_stopTls0
(JNIEnv *env, jobject thisObj)
{
	// Get a reference to this object's class
	jclass thisClass = env->GetObjectClass(thisObj);

	// Get the Field ID of the instance variable "cryptfd"
	jfieldID fidPlainfd = env->GetFieldID(thisClass, "plainfd", "I");
	if (NULL == fidPlainfd) return 0;
	// Get the int given the Field ID
	int plainfd = env->GetIntField(thisObj, fidPlainfd);
//fprintf(stderr, "stopTls0: plainfd = %d\n", plainfd);
	
#ifdef _WIN32
	closesocket(plainfd);
#else	
	close(plainfd);
#endif	
	return 0;
}

/*
 * Class:     nl_mansoft_tlspoolsocket_TlspoolSocket
 * Method:    shutdownWriteEncrypted
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_nl_mansoft_tlspoolsocket_TlspoolSocket_shutdownWriteEncrypted
  (JNIEnv *env, jobject thisObj)
{
	int rc;
	
	// Get a reference to this object's class
	jclass thisClass = env->GetObjectClass(thisObj);

	// Get the Field ID of the instance variable "cryptfd"
	jfieldID fidCryptfd = env->GetFieldID(thisClass, "cryptfd", "I");
	if (NULL == fidCryptfd) return -1;
	// Get the int given the Field ID
	int cryptfd = env->GetIntField(thisObj, fidCryptfd);
#ifdef _WIN32
	rc = shutdown(cryptfd, SD_SEND);
#else
	rc = shutdown(cryptfd, SHUT_WR);
#endif
	return rc;
}

#ifdef __cplusplus
}
#endif
