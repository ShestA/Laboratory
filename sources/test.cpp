// Copyright 2020 <DreamTeamGo>
#include <client_base.h>

int main()
{
	const char* names[] = { "John", "James", "Lucy", "Tracy", "Frank", "Abby", 0 };
	Client::create_clients(names, 6);
	return 0;
}