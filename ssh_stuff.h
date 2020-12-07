#include <string.h>
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
	switch (state) {
		case SSH_KNOWN_HOSTS_OK:
			/* OK */
			break;
		case SSH_KNOWN_HOSTS_CHANGED:
			fprintf(stderr, "Host key for server changed: it is now:\n");
			ssh_print_hash(SSH_PUBLICKEY_HASH_SHA256, hash, hlen);
			fprintf(stderr, "For security reasons, connection will be stopped\n");
			ssh_clean_pubkey_hash(&hash);
			return -1;
		case SSH_KNOWN_HOSTS_OTHER:
			fprintf(stderr, "The host key for this server was not found but an other type of key exists.\n");
			fprintf(stderr, "An attacker might change the default server key to confuse your client into thinking the key does not exist\n");
			ssh_clean_pubkey_hash(&hash);
			return -1;
		case SSH_KNOWN_HOSTS_NOT_FOUND:
			fprintf(stderr, "Could not find known host file.\n");
			fprintf(stderr, "If you accept the host key here, the file will be automatically created.\n");
		/* FALL THROUGH to SSH_SERVER_NOT_KNOWN behavior */
		case SSH_KNOWN_HOSTS_UNKNOWN:
			hexa = ssh_get_hexa(hash, hlen);
			fprintf(stderr,"The server is unknown. Do you trust the host key?\n");
			fprintf(stderr, "Public key hash: %s\n", hexa);
			ssh_string_free_char(hexa);
			ssh_clean_pubkey_hash(&hash);
			p = fgets(buf, sizeof(buf), stdin);
			if (p == NULL) {
				return -1;
			}
			cmp = strncasecmp(buf, "yes", 3);
			if (cmp != 0) {
				return -1;
			}
			rc = ssh_session_update_known_hosts(session);
			if (rc < 0) {
				fprintf(stderr, "Error %s\n", strerror(errno));
				return -1;
			}
			break;
		case SSH_KNOWN_HOSTS_ERROR:
			cerr << "Error: " << ssh_get_error(session) << endl;
			ssh_clean_pubkey_hash(&hash);
			return -1;
		}
		ssh_clean_pubkey_hash(&hash);
		return 0;
}

void connectSession(ssh_session session, string ip){
	ssh_options_set(session, SSH_OPTIONS_HOST, ip.c_str());
	if(ssh_connect(session) != SSH_OK){
		cerr << ip << ":   error connecting" << endl;
		return;
	}

	if(verify_knownhost(session) < 0){
		ssh_disconnect(session);
		return;
	}
	if(ssh_userauth_password(session, "admin", "adminpass") != SSH_AUTH_SUCCESS){
		cerr << "auth error ";
		cerr << ip << ":   " << ssh_get_error(session) << endl;
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
	while((nBytes = ssh_channel_read(channel, buffer, 256 * sizeof(char), 0)) > 0){
		if(write(1, buffer, nBytes) != nBytes){
			ssh_channel_close(channel);
			ssh_channel_free(channel);
			return SSH_ERROR;
		}
	}

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
