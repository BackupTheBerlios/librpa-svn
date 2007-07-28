// librpa/rpa.h
// 
// Copyright (C) 2007 Roman Kulikov

#ifndef __RPA_H__
#define __RPA_H__

#include <iostream>
#include <list>

// Exception codes
#define RPA_INT_INCORRECT_INP		1
#define RPA_INT_DIV_BY_ZERO		2
#define RPA_INT_INCORRECT_TYPE_CONV	3
#define RPA_INT_ALLOC_ERR		4

// So this class provides representation of Random Precision Arithmetic integer number
class rpa_int
{
	typedef unsigned short int		base_t;
	typedef unsigned long int		ext_t;

	typedef std::list< bool >		binbwn_t;	// binary bitwise notion
	typedef std::list< char >		decbwn_t;	// decimal bitwise notion
	typedef std::list< char >		hexbwn_t;	// heximal bitwise notion
	typedef std::pair< decbwn_t, bool >	divres_t;	// result of the division by 2
	
	friend std::istream& operator >> (std::istream& stream, rpa_int& obj);
	friend std::ostream& operator << (std::ostream& stream, rpa_int const& obj);
	friend std::ostream& operator << (std::ostream& stream, decbwn_t const& num);
	friend std::ostream& operator << (std::ostream& stream, binbwn_t const& num);
	
public:
	explicit rpa_int(long int num = 0);
	rpa_int(rpa_int const& obj);
	~rpa_int();

	long int		get_int() const;
	rpa_int&		operator = (rpa_int const& obj);

	rpa_int			operator << (int offset) const;
	rpa_int			operator >> (int offset) const;

	rpa_int			operator + (rpa_int const& obj) const;
	rpa_int			operator + (long int num) const;
	rpa_int			operator - (rpa_int const& obj) const;
	rpa_int			operator - (long int num) const;
	rpa_int			operator * (rpa_int const& obj) const;
	rpa_int			operator * (long int num) const;
	rpa_int			operator / (rpa_int const& obj) const;
	rpa_int			operator / (long int num) const;
	rpa_int			operator % (rpa_int const& obj) const;
	rpa_int			operator % (long int num) const;
	rpa_int&		operator ++ ();
	rpa_int&		operator -- ();
	rpa_int			operator ++ (int);
	rpa_int			operator -- (int);
	rpa_int&		operator += (rpa_int const& obj);
	rpa_int&		operator -= (rpa_int const& obj);
	rpa_int&		operator *= (rpa_int const& obj);
	rpa_int&		operator *= (long int num);
	rpa_int&		operator /= (rpa_int const& obj);
	rpa_int&		operator /= (long int num);

	bool			operator == (rpa_int const& obj) const;
	bool			operator == (long int num) const;
	bool			operator != (rpa_int const& obj) const;
	bool			operator != (long int num) const;
	bool			operator < (rpa_int const& obj) const;
	bool			operator < (long int num) const;
	bool			operator <= (rpa_int const& obj) const;
	bool			operator <= (long int num) const;
	bool			operator > (rpa_int const& obj) const;
	bool			operator > (long int num) const;
	bool			operator >= (rpa_int const& obj) const;
	bool			operator >= (long int num) const;
	
	rpa_int			abs() const;

	// greatest common divider
	rpa_int			gcd(rpa_int const& obj) const;

	// first - quotient, second -rest
	typedef std::pair< rpa_int, rpa_int >	div_res;

	void			conv_in_co2_code();
	div_res			divide(rpa_int const& obj) const;
	void			dump() const;

protected:
	static divres_t		divby2(decbwn_t dividend);
	static void		strip_high_zeros(decbwn_t& number);
	static void		strip_high_zeros(binbwn_t& number);

	static base_t const	s_BASE_T_MAX;
	static size_t const	s_BASE_T_SIZE;
	static ext_t const	s_EXT_T_MAX;
	static size_t const	s_EXT_T_SIZE;
	static size_t const	s_LONGINT_SIZE;

	void			strip();
	void			get_complement_masks(base_t& mask, base_t& spec_mask, size_t& spec_idx) const;
	void			realloc(size_t new_size, base_t const* src_num = 0);
	void			resize(size_t new_size, base_t const* src_num = 0);
	div_res			div_by_subtr(rpa_int const& obj) const;

private:
	bool			m_sign;
	size_t			m_size;
	base_t*			m_num;
};

bool operator == (long int i, rpa_int const& obj);
bool operator != (long int i, rpa_int const& obj);
bool operator > (long int i, rpa_int const& obj);
bool operator >= (long int i, rpa_int const& obj);
bool operator < (long int i, rpa_int const& obj);
bool operator <= (long int i, rpa_int const& obj);

rpa_int operator + (long int i, rpa_int const& obj);
rpa_int operator - (long int i, rpa_int const& obj);
rpa_int operator * (long int i, rpa_int const& obj);
rpa_int operator / (long int i, rpa_int const& obj);
rpa_int operator % (long int i, rpa_int const& obj);

std::ostream& operator << (std::ostream& stream, rpa_int::binbwn_t const& num);
std::ostream& operator << (std::ostream& stream, rpa_int::decbwn_t const& num);

#endif // #ifndef __RPA_H__

