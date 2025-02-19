Chat Server
This project implements a simple chat server using C and sockets. The server handles GET requests for retrieving chat messages and supports POST, EDIT, and REACT operations.

Features
Store Chats: Users can post messages with a username.
Retrieve Chats: Fetch all chat messages along with their timestamps.
Edit Messages: Modify an existing message using its ID.
React to Messages: Add reactions to messages with a username.
Reset Chat Storage: Clears all stored messages.
Implementation
Server (http-server.c):

Listens for incoming HTTP connections.
Routes requests based on the URL path (/chats, /post, /edit, /react, /reset).
Sends appropriate responses (200 OK, 400 Bad Request, 404 Not Found, 500 Internal Server Error).
Chat Storage (chat.c):

Uses a Chat structure to store messages, reactions, and timestamps.
Implements add_chat(), handle_edit(), and handle_react() to modify messages.
Manages a fixed-size storage (MAX_CHATS = 100,000).
Testing (test_add_chat()):

Runs assertions to verify proper chat handling.
Checks message length limits, empty parameters, and overflow handling.

API Endpoints
Method	Path	                                 Description
GET	   /chats	                              Fetch all chat messages.
GET	   /post?user=<name>&message=<msg>	      Add a chat message.
GET	   /edit?id=<id>&message=<msg>	          Edit a message by ID.
GET	   /react?id=<id>&user=<name>&message=<reaction>	React to a message.
GET	   /reset	                                 Clears all chat messages.


