#ifndef SSH_STUFF_H
#define SSH_STUFF_H

#include <string>
#include <string.h>

#include "send.h"

using namespace std;

extern pthread_mutex_t *mx;
int verify_knownhost(ssh_session session){
	enum ssh_known_hosts_e state;
	unsigned char *hash = NULL;
	ssh_key srv_pubkey = NULL;
	size_t hlen;
	char buf[10];
	char *hexa;
	char *p;
	int cmp;
	int rc;
	rc = ssh_get_server_publickey(session, &srv_pubkey);
	if (rc < 0) {
		return -1;
	}
	rc = ssh_get_publickey_hash(srv_pubkey, SSH_PUBLICKEY_HASH_SHA1, &hash, &hlen);
	ssh_key_free(srv_pubkey);
	if (rc < 0) {
		return -1;
	}
	state = ssh_session_is_known_server(session);
	string response;
	switch (state) {
		case SSH_KNOWN_HOSTS_OK:
			/* OK */
			break;
		case SSH_KNOWN_HOSTS_CHANGED:
			response.clear();
			response += "\nHost key for server changed: it is now:\n";
			response += "For security reasons, connection will be stopped";
			send(response);
			
			ssh_print_hash(SSH_PUBLICKEY_HASH_SHA256, hash, hlen);
			ssh_clean_pubkey_hash(&hash);
			return -1;
		case SSH_KNOWN_HOSTS_OTHER:
			response.clear();
			response += "\nThe host key for this server was not found but an other type of key exists.\n";
			response += "An attacker might change the default server key to confuse your client into thinking the key does not exist";
			send(response);
			ssh_clean_pubkey_hash(&hash);
			return -1;
		case SSH_KNOWN_HOSTS_NOT_FOUND:
			response.clear();
			response += "\nCould not find known host file.\n";
			response += "If you accept the host key here, the file will be automatically created.";
			send(response);
		/* FALL THROUGH to SSH_SERVER_NOT_KNOWN behavior */
		case SSH_KNOWN_HOSTS_UNKNOWN:
			hexa = ssh_get_hexa(hash, hlen);
			response.clear();
			response += "\nThe server is unknown. Adding to the list of trusted host keys.";
			response += "Public key hash: ";
			response += hexa;
			send(response);
			ssh_string_free_char(hexa);
			ssh_clean_pubkey_hash(&hash);
			rc = ssh_session_update_known_hosts(session);
			if (rc < 0) {
				response.clear();
				response += "\nError ";
				response += strerror(errno);
				send(response);
				return -1;
			}
			break;
		case SSH_KNOWN_HOSTS_ERROR:
			response.clear();
			response += "\nError: ";
			response += ssh_get_error(session);
			send(response);
			ssh_clean_pubkey_hash(&hash);
			return -1;
		}
		ssh_clean_pubkey_hash(&hash);
		return 0;
}

void connectSession(ssh_session session, string ip){
	ssh_options_set(session, SSH_OPTIONS_HOST, ip.c_str());
	if(ssh_connect(session) != SSH_OK){
		string response;
		response += '\n';
		response += ip;
		response += ":   error connecting";
		send(response);
		return;
	}

	if(verify_knownhost(session) < 0){
		ssh_disconnect(session);
		return;
	}
	if(ssh_userauth_password(session, "admin", "adminpass") != SSH_AUTH_SUCCESS){
		string response;
		response += "\nauth error\n";
		response += ip;
		response += ":   ";
		response += ssh_get_error(session);
		send(response);
		ssh_disconnect(session);
	}
}

int sshCommand(ssh_session session, const char * command){
	ssh_channel channel = ssh_channel_new(session);
	int rc;
	if(channel == NULL)
		return SSH_ERROR;
	if((rc = ssh_channel_open_session(channel)) != SSH_OK){
		ssh_channel_free(channel);
		return rc;
	}
	if((rc = ssh_channel_request_exec(channel, command)) != SSH_OK){
		ssh_channel_close(channel);
		ssh_channel_free(channel);
		return rc;
	}
	char buffer[256];
	int nBytes;
	pthread_mutex_lock(mx);
	nBytes = 1;
	write(1, &nBytes, sizeof(int));
	write(1, "\n", sizeof(char));
	while((nBytes = ssh_channel_read(channel, buffer, 256 * sizeof(char), 0)) > 0){
		write(1, &nBytes, sizeof(int));
		if(write(1, buffer, nBytes * sizeof(char)) != nBytes){
			ssh_channel_close(channel);
			ssh_channel_free(channel);
			pthread_mutex_unlock(mx);
			return SSH_ERROR;
		}
	}
	pthread_mutex_unlock(mx);

	if(nBytes < 0){
		ssh_channel_close(channel);
		ssh_channel_free(channel);
		return SSH_ERROR;
	}
	ssh_channel_send_eof(channel);
	ssh_channel_close(channel);
	ssh_channel_free(channel);
	return SSH_OK;
}

#endif