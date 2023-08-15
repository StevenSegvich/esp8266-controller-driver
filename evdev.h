struct packet{
	char flag;
	char aileron;
	char elevator;
	char rudder;
	char motor;
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
