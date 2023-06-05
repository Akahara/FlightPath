#ifndef H__ant_exception__
#define H__ant_exception__

class antException {
public:
	enum {
		TO_DELETE,
		TO_REGISTER
	};
	int state;
	TspAnt* a;
};

#endif
