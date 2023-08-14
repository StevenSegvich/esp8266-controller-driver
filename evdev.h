struct packet{
	char flag;
	char aileron[2];
	char elevator[2];
	char rudder[2];
	char motor[2];
	char endflag;
};

struct trim{
	int aileron;
	int elevator;
	int rudder;
	int motor;
};

struct fallback{
	int aileron;
	int elevator;
	int rudder;
	int motor;
};
