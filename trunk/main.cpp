// librpa/main.cpp
//
// Copyright (C) 2007 Roman Kulikov
//
// This is the set of trivial tests for rpa_int class


#include "rpa.h"

int main()
{
	rpa_int a;
	rpa_int b;

	std::cout << "Input a: ";
	std::cin >> a;
	std::cout << "Input b: ";
	std::cin >> b;

	std::cout << "a is: " <<  a << std::endl;
	std::cout << "b is: " << b << std::endl;

	std::cout << "GCD(a, b) = " << a.gcd(b) << std::endl;

	std::cout << "a == b : " << (a == b) << std::endl;
	std::cout << "a < b : " << (a < b) << std::endl;

	std::cout << "a == 65193465 : " << (a == 65193465) << std::endl;
	std::cout << "b < -619465 : " << (b < -619465) << std::endl;

	rpa_int c = a + b;
	std::cout << "c = a + b: " << c << std::endl;
	c = a - b;
	std::cout << "c = a - b: " << c << std::endl;

	rpa_int d = a * b;
	std::cout << "d = a * b: " << d << std::endl;

	rpa_int::div_res res = a.divide(b);
	std::cout << "a / b = " << res.first << "   a \% b = " << res.second << std::endl;
	
	std::cout << "++a is: " << ++a << std::endl;
	std::cout << "--b is: " << --b << std::endl;
	std::cout << "a++ is: " << a++ << " (" << a << ")" << std::endl;
	std::cout << "b-- is: " << b-- << " (" << b << ")" << std::endl;

	long int num = 0;
	std::cout << "Input long int: ";
	std::cin >> num;
	rpa_int e(num);
	std::cout << "e(long int): " << e << std::endl;
	std::cout << "(long int)e = " << e.get_int() << std::endl;

	return 0;
}

