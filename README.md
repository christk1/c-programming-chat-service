# c-programming-chat-service
compile servers.c---->
gcc servers.c -o servers  `mysql_config --cflags --libs` -lpthread

compile client.c----->
gcc -o client client.c -lpthread

start servers: ./servers
start a client in different terminal: ./client

instructions:
Initially user needs to sign up or login.

**When user logs in:**
* a menu with existing groups appears.
* a menu with services the main server can provide appears.

* write the number of service and hit enter.

**details**

* Users must first enroll before they can join a group.
* User that creates a group is automatically enrolled and has permissions to delete it.

* if users decide to enroll to a group, they must provide the id of that group(displayed when they make the request)
* if users want to join a group, all groups in which they are enrolled appear and they must provide the id of the preferred one.
* When users join a group, all messages of that group appear and they can start replying.
* Users can send private messages by typing @<username> <message>
* Users can close the connection by typing exit***

**Implementation details**
When users want to join a group, the main server checks for the capacity of a virtual child server and automatically place them in
a child server which is not full. The initial capacity of a virtual server is 2 users.

The database is mysql version 8+
