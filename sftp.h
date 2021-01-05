#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <sstream>
#include <stdio.h>
#include <errno.h>


// Good chunk size
#define MAX_XFER_BUF_SIZE 16384

void getDownloadParams(string command, string res[2]){
    istringstream ss(command);
    ss >> res[0];
    ss >> res[0];
    ss >> res[1];
}


string getFileName(string source){
	int i = source.rfind('/');
    int j = source.rfind('\\');
    i = max(i, j);
	return source.substr(i + 1);
}


int sftp_read_sync(ssh_session session, sftp_session sftp, string source, string dest)
{
    int access_type;
    sftp_file file;
    char buffer[MAX_XFER_BUF_SIZE];
    int nbytes, nwritten, rc;
    int fd;
    bool needFree = 0;

    access_type = O_RDONLY;
    file = sftp_open(sftp, source.c_str(),
                     access_type, 0);
    sftp_attributes attr = sftp_fstat(file);
    int x = 0;
    pthread_mutex_lock(mx);
    write(1, &x, sizeof(int));
    write(1, &(attr->size), sizeof(uint64_t));
    sftp_attributes_free(attr);
    if (file == NULL)
    {
        fprintf(stderr, "Can't open file for reading: %s\n",
                ssh_get_error(session));
        x = -1;
        write(1, &x, sizeof(int));
        pthread_mutex_unlock(mx);
        return SSH_ERROR;
    }
    if(dest.empty())
        dest = getFileName(source);

    fd = open(dest.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (fd < 0)
    {
        fprintf(stderr, "Can't open file for writing: %s\n",
                strerror(errno));
        x = -1;
        write(1, &x, sizeof(int));
        pthread_mutex_unlock(mx);
        return SSH_ERROR;
    }

    for (;;)
    {
        nbytes = sftp_read(file, buffer, sizeof(buffer));
        if (nbytes == 0)
        {
            break; // EOF
        }
        else if (nbytes < 0)
        {
            fprintf(stderr, "Error while reading file: %s\n",
                    ssh_get_error(session));
            sftp_close(file);
            x = -1;
            write(1, &x, sizeof(int));
            pthread_mutex_unlock(mx);
            return SSH_ERROR;
        }

        nwritten = write(fd, buffer, nbytes);
        write(1, &nwritten, sizeof(int));
        if (nwritten != nbytes)
        {
            fprintf(stderr, "Error writing: %s\n",
                    strerror(errno));
            sftp_close(file);
            x = -1;
            write(1, &x, sizeof(int));
            pthread_mutex_unlock(mx);
            return SSH_ERROR;
        }
    }
    pthread_mutex_unlock(mx);

    rc = sftp_close(file);
    if (rc != SSH_OK)
    {
        fprintf(stderr, "Can't close the read file: %s\n",
                ssh_get_error(session));
        return rc;
    }
    close(fd);
    string res = "\nFile downloaded successfully";
    send(res);

    return SSH_OK;
}