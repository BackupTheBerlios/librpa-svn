// librpa/rpa.cpp
//
// Copyright (C) 2007 Roman Kulikov

#include "rpa.h"
#include <string>
#include <limits.h>

#define DECR_BASE_T_PTR(x)	x = (rpa_int::base_t*)((int)(x) - 1)

#define SWAP_BASE_T_PTRS(x,y)	x = (rpa_int::base_t*)((int)(x) ^ (int)(y)); \
				y = (rpa_int::base_t*)((int)(y) ^ (int)(x)); \
				x = (rpa_int::base_t*)((int)(x) ^ (int)(y));

#define WRITE_BYTE(dst_ptr,src)	{ char buf = *(char*)(dst_ptr); \
	 			  *(char*)(dst_ptr) = ((buf) & 0xFFFFFF00) | ((src) & 0xFF); }

#define LOW_BITS_MASK(x,n)	x = (1 << (n)) - 1;

#define HIGH_BITS_MASK(x,n)	LOW_BITS_MASK(x,n) \
				x <<= (s_BASE_T_SIZE - n);

#define MIN(x,y)		((x) < (y) ? (x) : (y))
#define MAX(x,y)		((x) > (y) ? (x) : (y))


#define BASE_T_MAX 		USHRT_MAX
#define EXT_T_MAX		ULONG_MAX

rpa_int::base_t const rpa_int::s_BASE_T_MAX = BASE_T_MAX;
rpa_int::ext_t const rpa_int::s_EXT_T_MAX = EXT_T_MAX;
size_t const rpa_int::s_EXT_T_SIZE = 8 * sizeof(rpa_int::ext_t);
size_t const rpa_int::s_BASE_T_SIZE = 8 * sizeof(rpa_int::base_t);
size_t const rpa_int::s_LONGINT_SIZE = 8 * sizeof(long int);

// ----------------------------------------------------------------------------------------------------------------------------------------
rpa_int::rpa_int(long int num):
	m_sign(num < 0),
	m_size(s_LONGINT_SIZE / s_BASE_T_SIZE),
	m_num(new base_t[s_LONGINT_SIZE / s_BASE_T_SIZE])
{
#ifndef DISABLE_EXCEPTIONS
	if (0 == m_num)
	{
		throw RPA_INT_ALLOC_ERR;
	}
#endif
	if (num < 0)
	{
		num *= -1;
	}

	for (size_t i = 0; i < m_size; ++i)
	{
		m_num[i] = (base_t)num;
		num >>= s_BASE_T_SIZE;
	}

	strip();
}

// ----------------------------------------------------------------------------------------------------------------------------------------
rpa_int::rpa_int(rpa_int const& obj):
	m_sign(obj.m_sign),
	m_size(obj.m_size),
	m_num(new base_t[obj.m_size])
{
#ifndef DISABLE_EXCEPTIONS
	if (0 == m_num)
	{
		throw RPA_INT_ALLOC_ERR;
	}
#endif
	memcpy(m_num, obj.m_num, sizeof(base_t) * m_size);
}

// ----------------------------------------------------------------------------------------------------------------------------------------
rpa_int::~rpa_int()
{
	delete[] m_num;
}

// ----------------------------------------------------------------------------------------------------------------------------------------
std::istream& operator >> (std::istream& stream, rpa_int& obj)
{
	std::string inp_string;
	stream >> inp_string;

	if (0 == inp_string.size())
	{
		return stream;
	}
	
	// reading sign of the number

	if ('-' == inp_string[0])
	{
		obj.m_sign = true;
		inp_string = inp_string.substr(1, std::string::npos);
	}
	else
	{
		obj.m_sign = false;
		if ('+' == inp_string[0])
		{
			inp_string = inp_string.substr(1, std::string::npos);
		}
	}

	if (0 == inp_string.size())
	{
#ifdef DISABLE_EXCEPTIONS
		exit(1);
#else
		throw RPA_INT_INCORRECT_INP;
#endif
	}

	rpa_int::decbwn_t inp_dec_figs;	// decimal figures read from
					// input text stream

	// forming decimal bitwise notion of the number
	
	for (std::string::size_type i = 0; i < inp_string.size(); ++i)
	{
		if (isdigit(inp_string[i]))
		{
			inp_dec_figs.push_back(inp_string[i] - 48);
		}
		else
		{
#ifdef DISABLE_EXCEPTIONS
			exit(1);
#else
			throw RPA_INT_INCORRECT_INP;
#endif
		}
	}

	rpa_int::binbwn_t bits;
	rpa_int::divres_t tmp_res;

	// converting decimal bitwise notion into binary
	// using standard algoriyhm of long division

	while (true)
	{
		tmp_res = rpa_int::divby2(inp_dec_figs);
		bits.push_front(tmp_res.second);
		if (1 == tmp_res.first.size() && *tmp_res.first.begin() < 2)
		{
			bits.push_front(*tmp_res.first.begin());
			break;
		}
		inp_dec_figs = tmp_res.first;
	}

	rpa_int::strip_high_zeros(bits);

	/**
	rpa_int::divres_t res = rpa_int::divby2(inp_dec_figs);

	for (rpa_int::decbwn_t::const_iterator it = res.first.begin(); it != res.first.end(); ++it)
	{
		std::cout << "> " << (int)*it << std::endl;
	}
	*/

	/**
	for (rpa_int::binbwn_t::const_iterator it = bits.begin(); it != bits.end(); ++it)
	{
		std::cout << (int)*it;
	}
	std::cout << std::endl;
	*/

	// reallocating memory of object

	delete[] obj.m_num;
	obj.m_size = bits.size() / rpa_int::s_BASE_T_SIZE;
	if (bits.size() % rpa_int::s_BASE_T_SIZE)
	{
		++obj.m_size;
	}
	obj.m_num = new rpa_int::base_t[obj.m_size];
	memset(obj.m_num, 0, sizeof(rpa_int::base_t) * obj.m_size);

	// writing bits into number

	int i = 0;
	for (rpa_int::binbwn_t::reverse_iterator rit = bits.rbegin(); rit != bits.rend(); ++rit, ++i)
	{
		if (*rit)
		{
			obj.m_num[i / rpa_int::s_BASE_T_SIZE] |= 1 << (i % rpa_int::s_BASE_T_SIZE);
		}
	}

	if (1 == obj.m_size && 0 == obj.m_num[0])
	{
		obj.m_sign = false;
	}

	return stream;
}

// ----------------------------------------------------------------------------------------------------------------------------------------
std::ostream& operator << (std::ostream& stream, rpa_int const& obj)
{
	if (obj.m_sign)
	{
		stream << "-";
	}
	rpa_int::decbwn_t dec_figs;

	rpa_int::base_t* divident = new rpa_int::base_t[obj.m_size];
	memcpy(divident, obj.m_num, sizeof(rpa_int::base_t) * obj.m_size);
	rpa_int::base_t* quotient = new rpa_int::base_t[obj.m_size];
	memset(quotient, 0, sizeof(rpa_int::base_t) * obj.m_size);
	rpa_int::base_t* stop_flag = divident;
	rpa_int::base_t* next_stop_flag = 0;
	bool bytes_even = true;

	while (true)
	{
		bool was_operation = false;
		rpa_int::base_t* d = divident + obj.m_size;
		rpa_int::base_t* q = quotient + obj.m_size;
		unsigned char curr_byte = 0;
		unsigned char half_byte = 0;
		unsigned char carry_byte = 0;
		unsigned char low_quart = 0;
		unsigned char high_quart = 0;
		unsigned char res_byte = 0;
		bool res_empty = true;

		DECR_BASE_T_PTR(d);
		DECR_BASE_T_PTR(q);
		while (true)
		{
			curr_byte = (unsigned char)*d;
			low_quart = 0xF & curr_byte;
			half_byte = curr_byte >> 4;
			high_quart =  0xF & half_byte;
			high_quart += carry_byte;

			if (res_empty)
			{
				res_byte = (high_quart / 0xA) << 4;
				res_empty = false;
			}
			else
			{
				res_byte += high_quart / 0xA;
				WRITE_BYTE(q, res_byte);				
				next_stop_flag = q;
				DECR_BASE_T_PTR(q);
				res_empty = true;
			}
			if (high_quart < 0xA)
			{
				carry_byte = high_quart << 4;
			}
			else
			{
				was_operation = true;
				carry_byte = (high_quart % 0xA) << 4;
			}

			DECR_BASE_T_PTR(d);
			bool break_loop = false;
			if (d < stop_flag)
			{
				break_loop = true;
			}

			if (bytes_even || !break_loop)
			{
				low_quart += carry_byte;
				if (res_empty)
				{
					res_byte = (low_quart / 0xA) << 4;
					res_empty = false;
				}
				else
				{
					res_byte += low_quart / 0xA;
					WRITE_BYTE(q, res_byte);
					next_stop_flag = q;
					DECR_BASE_T_PTR(q);
					res_empty = true;
				}
				if (low_quart < 0xA)
				{
					carry_byte = low_quart << 4;
				}
				else
				{
					was_operation = true;
					carry_byte = (low_quart % 0xA) << 4;
				}
			}
			if (break_loop)
			{
				break;
			}                                         			
		}

		// ----------------------------------

		dec_figs.push_front(carry_byte >>= 4);
		if (was_operation)
		{
			if (!res_empty)
			{
				*q = res_byte;
				bytes_even = false;
			}
			else
			{
				bytes_even = true;
			}
			SWAP_BASE_T_PTRS(divident, quotient);
			stop_flag = next_stop_flag;
			memset(quotient, 0, sizeof(rpa_int::base_t) * obj.m_size);
		}
		else
		{
			dec_figs.push_front(quotient[0] & 0xF);
			break;
		}
	}

	delete[] divident;
	delete[] quotient;

	rpa_int::strip_high_zeros(dec_figs);
	stream << dec_figs;

	return stream;
}

// ----------------------------------------------------------------------------------------------------------------------------------------
rpa_int& rpa_int::operator = (rpa_int const& obj)
{
	realloc(obj.m_size, obj.m_num);
	m_sign = obj.m_sign;

	return *this;
}

// ----------------------------------------------------------------------------------------------------------------------------------------
rpa_int rpa_int::operator << (int offset) const
{
	if (offset < 0)
	{
		return *this >> (-1 * offset);
	}
	else if (offset == 0)
	{
		return *this;
	}

	rpa_int new_num;

	if ((size_t)offset > s_BASE_T_SIZE)
	{
		// This part of code should be studied carefully.
		// Here's much to optimize and make prettier.
		new_num = *this;
		int n = offset / s_BASE_T_SIZE;
		for (int i = 0; i < n; ++i)
		{
			new_num = new_num << s_BASE_T_SIZE;
		}
		return new_num << (offset % s_BASE_T_SIZE);
	}

	base_t offset_bit_mask;
	HIGH_BITS_MASK(offset_bit_mask, offset)

	base_t highest_bit_mask = 1 << (s_BASE_T_SIZE - 1);
	int num_high_zeros = 0;
	base_t high_bytes = m_num[m_size - 1];
	while (!(high_bytes & highest_bit_mask))
	{
		high_bytes <<= 1;
		++num_high_zeros;
	}

	if (num_high_zeros >= offset)
	{
		// we don't need to allocate new memory for new number
		new_num.m_size = m_size;
	}
	else
	{
		new_num.m_size = m_size + 1;
	}

	delete[] new_num.m_num;
	new_num.m_num = new base_t[new_num.m_size];
	new_num.m_sign = m_sign;
	size_t i = m_size;
	do
	{
		--i;
		new_num.m_num[i] = m_num[i] << offset;
		if (i > 0)
		{
			new_num.m_num[i] |= (m_num[i - 1] & offset_bit_mask) >> (s_BASE_T_SIZE - offset);
		}
	} while (i > 0);

	if (num_high_zeros < offset)
	{
		base_t carry_bits_mask;
		HIGH_BITS_MASK(carry_bits_mask, offset - num_high_zeros);
		new_num.m_num[new_num.m_size - 1] =
			m_num[m_size - 1] & carry_bits_mask >> (s_BASE_T_SIZE - offset + num_high_zeros);
	}

	// But I think it's not really necessary here.
	new_num.strip();

	return new_num;
}

// ----------------------------------------------------------------------------------------------------------------------------------------
rpa_int rpa_int::operator >> (int offset) const
{
	if (offset < 0)
	{
		return *this << (-1 * offset);
	}

	if ((size_t)offset >= m_size * s_BASE_T_SIZE)
	{
		return rpa_int(0);
	}

	rpa_int new_num;

	if ((size_t)offset > s_BASE_T_SIZE)
	{
		new_num = *this;
		int n = offset / s_BASE_T_SIZE;
		for (int i = 0; i < n; ++i)
		{
			new_num = new_num >> s_BASE_T_SIZE;
		}
		return new_num >> (offset % s_BASE_T_SIZE);
	}
	
	delete[] new_num.m_num;
	new_num.m_size = m_size;
	new_num.m_sign = m_sign;
	new_num.m_num = new base_t[new_num.m_size];

	base_t offset_bits_mask;
	LOW_BITS_MASK(offset_bits_mask, offset)
	
	for (size_t i = 0; i < m_size; ++i)
	{
		new_num.m_num[i] = m_num[i] >> offset;
		if (i < m_size - 1)
		{
			new_num.m_num[i] |= (m_num[i + 1] & offset_bits_mask) << (s_BASE_T_SIZE - offset);
		}
	}

	new_num.strip();

	return new_num;
}

// ----------------------------------------------------------------------------------------------------------------------------------------
rpa_int::divres_t rpa_int::divby2(decbwn_t divident)
{
	decbwn_t quotient;
	char curr_number = 0;
	decbwn_t::iterator it = divident.begin();
	bool rest = 0;
	do
	{
		curr_number = *it;
		quotient.push_back(curr_number / 2);
		++it;
		if (curr_number % 2)
		{
			if (it != divident.end())
			{
				*it += 10;
			}
			else
			{
				rest = curr_number;
				break;
			}
		}
	} while (it != divident.end());

	strip_high_zeros(quotient);

	return divres_t(quotient, rest);
}

// ----------------------------------------------------------------------------------------------------------------------------------------
void rpa_int::strip_high_zeros(decbwn_t& number)
{
	for (decbwn_t::iterator it = number.begin(); it != number.end(); ++it)
	{
		if (*it == 0)
		{
			number.erase(it++);
			--it;
		}
		else
		{
			break;
		}
	}
	if (number.empty())
	{
		number.push_back(0);
	}
}

// ----------------------------------------------------------------------------------------------------------------------------------------
void rpa_int::strip_high_zeros(binbwn_t& number)
{
	for (binbwn_t::iterator it = number.begin(); it != number.end(); ++it)
	{
		if (*it == 0)
		{
			number.erase(it++);
			--it;
		}
		else
		{
			break;
		}
	}
	if (number.empty())
	{
		number.push_back(0);
	}
}

// ----------------------------------------------------------------------------------------------------------------------------------------
void rpa_int::dump() const
{
	binbwn_t output_bits;

	base_t curr_item = 0;
	base_t bit_mask = 0;
	for (size_t i = 0; i < m_size; ++i)
	{
		curr_item = m_num[i];
		bit_mask = 1;
		for (size_t j = 1; j <= s_BASE_T_SIZE; ++j, bit_mask <<= 1)
		{
			(curr_item & bit_mask) ?
				output_bits.push_front(true) :
				output_bits.push_front(false);
		}
	}

	if (m_sign)
	{
		std::cout << "-";
	}
	std::cout << output_bits << std::endl;
}

// ----------------------------------------------------------------------------------------------------------------------------------------
std::ostream& operator << (std::ostream& stream, rpa_int::binbwn_t const& number)
{
	short byte_cntr = 8 - number.size() % 8;
	for (rpa_int::binbwn_t::const_iterator i = number.begin(); i != number.end(); ++i, ++byte_cntr)
	{
		if (byte_cntr == 8)
		{
			stream << '.';
			byte_cntr = 0;
		}
		stream << (int)*i;
	}

	return stream;
}

// ----------------------------------------------------------------------------------------------------------------------------------------
std::ostream& operator << (std::ostream& stream, rpa_int::decbwn_t const& number)
{
	for (rpa_int::decbwn_t::const_iterator i = number.begin(); i != number.end(); ++i)
	{
		stream << (int)*i;
	}

	return stream;
}

// ----------------------------------------------------------------------------------------------------------------------------------------
void rpa_int::strip()
{
	size_t num_zero_items = 0;
	size_t i = m_size;
	do
	{
		--i;
		if (m_num[i] == 0)
		{
			++num_zero_items;
		}
		else
		{
			break;
		}
	} while (i > 0);

	if (num_zero_items == m_size)
	{
		// our number is zero
		realloc(1);
	}
	else
	{
		base_t* new_num = new base_t[m_size - num_zero_items];
#ifndef DISABLE_EXCEPTIONS
		if (0 == new_num)
		{
			throw RPA_INT_ALLOC_ERR;
		}
#endif
		memcpy(new_num, m_num, sizeof(base_t) * (m_size - num_zero_items));
		delete[] m_num;
		m_num = new_num;
		m_size -= num_zero_items;
	}
}

// ----------------------------------------------------------------------------------------------------------------------------------------
void rpa_int::realloc(size_t new_size, base_t const* src_num)
{
	delete[] m_num;
	m_num = new base_t[new_size];
#ifndef DISABLE_EXCEPTIONS
	if (0 == m_num)
	{
		throw RPA_INT_ALLOC_ERR;
	}
#endif
	m_size = new_size;
	if (0 == src_num)
	{
		memset(m_num, 0, sizeof(base_t) * m_size);
	}
	else
	{
		memcpy(m_num, src_num, sizeof(base_t) * m_size);
	}
	m_sign = false;
}

// ----------------------------------------------------------------------------------------------------------------------------------------
void rpa_int::resize(size_t new_size, base_t const* src_num)
{
	base_t* resized_num = new base_t[new_size];
#ifndef DISABLE_EXCEPTIONS
	if (0 == resized_num)
	{
		throw RPA_INT_ALLOC_ERR;
	}
#endif
	memcpy(resized_num, m_num, sizeof(base_t) * m_size);
	delete[] m_num;
	m_num = resized_num;
	if (new_size > m_size)
	{
		if (0 == src_num)
		{
			memset(&(m_num[m_size]), 0, sizeof(base_t) * (new_size - m_size));
		}
		else
		{
			memcpy(&(m_num[m_size]), src_num, sizeof(base_t) * (new_size - m_size));
		}
	}
	m_size = new_size;
}

// ----------------------------------------------------------------------------------------------------------------------------------------
void rpa_int::conv_in_co2_code()
{
	base_t bit_mask = 0;		// bitmask for all items from left side of item
					// which contain the rightmost bit 1.
	base_t spec_bit_mask = 0;	// mask for item, containing the rightmost bit 1.
	size_t spec_idx = m_size - 1;	// index of such item
	get_complement_masks(bit_mask, spec_bit_mask, spec_idx);
	size_t i = m_size - 1;
	while (i > spec_idx)
	{
		m_num[i] ^= bit_mask;
		--i;
	}
	m_num[spec_idx] ^= spec_bit_mask;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// B O O L E A N   O P E R A T I O N S
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------------------------------------------------------------------
bool rpa_int::operator == (rpa_int const& obj) const
{
	if (this == &obj)
	{
		return true;
	}

	if (m_sign != obj.m_sign || m_size != obj.m_size)
	{
		return false;
	}

	for (size_t i = 0; i < m_size; ++i)
	{
		if (m_num[i] != obj.m_num[i])
		{
			return false;
		}
	}

	return true;
}

// ----------------------------------------------------------------------------------------------------------------------------------------
bool rpa_int::operator == (long int num) const
{
	if (num == 0 && m_size == 1 && m_num[0] == 0)
	{
		return true;
	}
	
	if (m_sign && num >= 0 || !m_sign && num < 0)
	{
		return false;
	}

	if (m_size * s_BASE_T_SIZE > s_LONGINT_SIZE)
	{
		return false;
	}

	if (num < 0)
	{
		num *= -1;
	}

	long int bit_mask;
	if (m_size * s_BASE_T_SIZE == s_LONGINT_SIZE)
	{
		// In this case we are not able to get
		// correct bit mask using LOW_BITS_MASK macro.
		bit_mask = LONG_MAX;
	}
	else
	{
		LOW_BITS_MASK(bit_mask, m_size * s_BASE_T_SIZE);
	}

	return num == ((*(long int*)m_num) & bit_mask);
}

// ----------------------------------------------------------------------------------------------------------------------------------------
bool rpa_int::operator != (rpa_int const& obj) const
{
	return !(*this == obj);
}

// ----------------------------------------------------------------------------------------------------------------------------------------
bool rpa_int::operator != (long int num) const
{
	return !(*this == num);
}

// ----------------------------------------------------------------------------------------------------------------------------------------
bool rpa_int::operator < (rpa_int const& obj) const
{
	if (this == &obj)
	{
		return false;
	}
	if (m_sign && !obj.m_sign)
	{
		return true;
	}
	if (!m_sign && obj.m_sign)
	{
		return false;
	}

	if (m_sign)
	{
		// We compare two negative numbers. So the operator must return true,
		// if absolute part of *this is greater.
		if (m_size > obj.m_size)
		{
			return true;
		} else if (m_size < obj.m_size)
		{
			return false;
		}

		size_t i = m_size;
		do
		{
			--i;
			if (m_num[i] > obj.m_num[i])
			{
				return true;
			}
			else if (m_num[i] < obj.m_num[i])
			{
				return false;
			}
		} while (i > 0);
	
		// numbers are equal
		return false;
	}

	// Both numbers are positive. So we search for less.
	
	if (m_size > obj.m_size)
	{
		return false;
	} else if (m_size < obj.m_size)
	{
		return true;
	}

	size_t i = m_size;
	do
	{
		--i;
		if (m_num[i] > obj.m_num[i])
		{
			return false;
		}
		else if (m_num[i] < obj.m_num[i])
		{
			return true;
		}
	} while (i > 0);
	
	// numbers are equal
	return false;
}

// ----------------------------------------------------------------------------------------------------------------------------------------
bool rpa_int::operator < (long int num) const
{
	if (m_sign && num >= 0)
	{
		return true;
	}
	if (!m_sign && num < 0)
	{
		return false;
	}

	if (m_size * s_BASE_T_SIZE > s_LONGINT_SIZE)
	{
		return false;
	}

	// Here we use m_num as pointer to long int. But if the total size of all items of m_num
	// is less then the size of long int, operation "*(long int*)m_num" may take some unneceassary
	// bits after m_num array. So we need to supress this bits by special mask bit_mask. 
	
	long int bit_mask = 0;
	if (m_size * s_BASE_T_SIZE == s_LONGINT_SIZE)
	{
		// In this case we are not able to get
		// correct bit mask using LOW_BITS_MASK macro.
		bit_mask = LONG_MAX;
	}
	else
	{
		LOW_BITS_MASK(bit_mask, m_size * s_BASE_T_SIZE);
	}

	if (num < 0)
	{
		num *= -1;
		return ((*(long int*)m_num) & bit_mask) > num;
	}

	return ((*(long int*)m_num) & bit_mask) < num;
}

// ----------------------------------------------------------------------------------------------------------------------------------------
bool rpa_int::operator <= (rpa_int const& obj) const
{
	return *this < obj || *this == obj;
}

// ----------------------------------------------------------------------------------------------------------------------------------------
bool rpa_int::operator <= (long int num) const
{
	return *this < num || *this == num;
}

// ----------------------------------------------------------------------------------------------------------------------------------------
bool rpa_int::operator > (rpa_int const& obj) const
{
	return !(*this <= obj);
}

// ----------------------------------------------------------------------------------------------------------------------------------------
bool rpa_int::operator > (long int num) const
{
	return !(*this <= num);
}

// ----------------------------------------------------------------------------------------------------------------------------------------
bool rpa_int::operator >= (rpa_int const& obj) const
{
	return !(*this < obj);
}

// ----------------------------------------------------------------------------------------------------------------------------------------
bool rpa_int::operator >= (long int num) const
{
	return !(*this < num);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// A R I T H M E T I C   O P E R A T I O N S
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------------------------------------------------------------------
rpa_int rpa_int::operator + (rpa_int const& obj) const
{
	rpa_int ret_num(*this);
	ret_num += obj;

	return ret_num;
}

// ----------------------------------------------------------------------------------------------------------------------------------------
rpa_int	rpa_int::operator + (long int num) const
{
	return *this + rpa_int(num);
}

// ----------------------------------------------------------------------------------------------------------------------------------------
rpa_int& rpa_int::operator += (rpa_int const& obj)
{
	if (obj == 0)
	{
		return *this;
	}
	
	// If the number is negative, we must convert it into complement-on-two code.
	// But we won't do the convertion for the whole number. We'll do it for each
	// exact item.
	// 
	// Here are bit masks for XOR-operatrion.
	// So if bit of such mask has 1, then xor inverts appropriate bit of maked item.
	// If bit of mask is 0, appropriate bit staies unchanged.
	// 
	// The algorithm of convertion to complement code is the simplest: we invert all bits
	// starting from the heighest ones whilst we reach the rightmost bit 1. Then we stop.
	// The rightmost bit 1 stays unchanged.

	resize(MAX(m_size, obj.m_size) + 1, 0);

	if (m_sign)
	{
		conv_in_co2_code();
	}

	// masks for convertion right argument into complement-on-two code
	base_t bit_mask_right = 0;
	base_t spec_bit_mask_right = 0;
	size_t spec_idx_right = m_size - 1;
	obj.get_complement_masks(bit_mask_right, spec_bit_mask_right, spec_idx_right);

	// Just an item-by-item summation in complement-on-two code.
	
	ext_t right_op = 0;
	ext_t carry = 0;
	size_t i;
	for (i = 0; i < m_size - 1; ++i)
	{
		right_op = 0;
	
		if (i < obj.m_size)
		{
			right_op = obj.m_num[i];
		}
		if (i == spec_idx_right)
		{
			right_op ^= spec_bit_mask_right;
		}
		else if (i > spec_idx_right)
		{
			right_op ^= bit_mask_right;
		}

		carry = m_num[i] + (base_t)right_op + (base_t)carry;
		m_num[i] = carry;
		carry >>= s_BASE_T_SIZE;		// throwing away the first half of bits.
							// so only carry bit stays
	}
	m_num[i] += carry + bit_mask_right;		// carry to the highest item

	// At this moment we recieve result in complement-on-two code.
	// So if the number is negative (the heighest bit is 1)
	// we have to produce the back convertion into common code.
	if (m_num[i] & (1 << (s_BASE_T_SIZE - 1)))
	{
		m_sign = true;
		conv_in_co2_code();
	}
	else
	{
		m_sign = false;
	}

	strip();

	return *this;
}

// ----------------------------------------------------------------------------------------------------------------------------------------
rpa_int	rpa_int::operator - (rpa_int const& obj) const
{
	if (*this == obj)
	{
		return rpa_int(0);
	}

	return *this + obj * (-1);
}

// ----------------------------------------------------------------------------------------------------------------------------------------
rpa_int	rpa_int::operator - (long int num) const
{
	return *this - rpa_int(num);
}

// ----------------------------------------------------------------------------------------------------------------------------------------
rpa_int& rpa_int::operator -= (rpa_int const& obj)
{
	return operator += (obj * (-1));
}

// ----------------------------------------------------------------------------------------------------------------------------------------
rpa_int	rpa_int::operator * (rpa_int const& obj) const
{
	if (obj == 0)
	{
		return rpa_int(0);
	}
	else if (obj == -1)
	{
		rpa_int new_num(*this);
		new_num.m_sign = !new_num.m_sign;
		return new_num;
	}
	else if (obj == 1)
	{
		return *this;
	}

	rpa_int new_num;
	new_num.realloc(m_size + obj.m_size);
	new_num.m_sign = m_sign != obj.m_sign;

	// Here we use algorithm of "long multiplication".
	// Here is an idea that result of multiplication of two
	// variables of base_t type comes into ext_t type completely.

	ext_t res = 0;
	ext_t carry = 0;
	ext_t const carry_mask = 1 << s_BASE_T_SIZE;	// Yes. This carry mask has such
							// strange value because of it is
							// applied to the value on the next
							// iteration, i.e. current item has
							// s_BASE_T_SIZE bits offset from
							// previous iteration.
	for (size_t i = 0; i < obj.m_size; ++i)
	{
		for (size_t j = 0; j < m_size; ++j)
		{
			ext_t& curr_res_item = *((ext_t*)(&(new_num.m_num[i + j])));
			res = (ext_t)m_num[j] * (ext_t)obj.m_num[i] + carry;
			if (s_EXT_T_MAX - curr_res_item < res)
			{
				carry = carry_mask;
			}
			else
			{
				carry = 0;
			}
			curr_res_item += res;
		}
		if (carry)
		{
			// This is strange place really.
			// What we are doing here?
			if (i + m_size + 1 < new_num.m_size)
			{
				new_num.m_num[i + m_size + 1] += 1;
			}
		}
	}

	new_num.strip();

	return new_num;
}

// ----------------------------------------------------------------------------------------------------------------------------------------
rpa_int	rpa_int::operator * (long int num) const
{
	return *this * rpa_int(num);
}

// ----------------------------------------------------------------------------------------------------------------------------------------
rpa_int& rpa_int::operator *= (rpa_int const& obj)
{
	*this = operator * (obj);
	return *this;
}

rpa_int& rpa_int::operator *= (long int num)
{
	*this = operator * (num);
	return *this;
}

// ----------------------------------------------------------------------------------------------------------------------------------------
rpa_int	rpa_int::operator / (rpa_int const& obj) const
{
	if (obj == 0)
	{
#ifdef DISABLE_EXCEPTIONS
		return rpa_int();
#else
		throw RPA_INT_DIV_BY_ZERO;
#endif
	}
	else if (*this == obj)
	{
		return rpa_int(1);
	}
	else if (*this == obj * (-1))
	{
		return rpa_int(-1);
	}

	return divide(obj).first;
}

// ----------------------------------------------------------------------------------------------------------------------------------------
rpa_int& rpa_int::operator /= (rpa_int const& divisor)
{
	if (divisor == 0)
	{
#ifdef DISABLE_EXCEPTIONS
		*this;
#else
		throw RPA_INT_DIV_BY_ZERO;
#endif
	}

	*this = operator / (divisor);

	return *this;
}

// ----------------------------------------------------------------------------------------------------------------------------------------
rpa_int& rpa_int::operator /= (long int divisor)
{
	if (divisor == 0)
	{
#ifdef DISABLE_EXCEPTIONS
		return *this;
#else
		throw RPA_INT_DIV_BY_ZERO;
#endif
	}

	*this = operator / (divisor);

	return *this;
}

// ----------------------------------------------------------------------------------------------------------------------------------------
rpa_int rpa_int::operator % (rpa_int const& obj) const
{
	return divide(obj).second;
}

// ----------------------------------------------------------------------------------------------------------------------------------------
rpa_int rpa_int::operator % (long int num) const
{
	return divide(rpa_int(num)).second;
}

// ----------------------------------------------------------------------------------------------------------------------------------------
rpa_int::div_res rpa_int::divide(rpa_int const& divisor) const
{
	if (m_size < divisor.m_size)
	{
		// divident is less than divisor
		rpa_int rest = *this;
		rest.m_sign = m_sign != divisor.m_sign;
		return div_res(rpa_int(0), rest);
	}

	// It is the simplest algorithm of long division. Stupid and
	// without any otimizations and brilliant ides.
	//
	// That's why it works very slow :-(
	//
	// Good idea is to multiplay divisor on, for example, two in
	// some power to make better alignment of divident and divisor size
	// and, as a result, reduce the number of iterations.
	
	std::list< base_t > quotient_list;	// here we'll store items of the quotient
						// in the reverse sequrnce
	rpa_int rest;
	div_res curr_div_res;
	rpa_int curr_divident;
	size_t curr_idx = m_size - divisor.m_size; // current index of idet in divident

	if (curr_idx < m_size)
	{
		rest.realloc(m_size - curr_idx - 1, &m_num[curr_idx + 1]);
	}

	while (true)
	{
		curr_divident = rpa_int(m_num[curr_idx]);
		if (rest != 0)
		{
			// we recieved rest from the previous iteration.
			// so we have to add it (not add but use as high bits) to the current divident
			curr_divident.resize(curr_divident.m_size + rest.m_size, &(rest.m_num[0]));
		}
		curr_div_res = curr_divident.div_by_subtr(divisor);
		// we use only zero item cause we sure that there is no any in this quotient -
		//  this is algorithmical feature
		quotient_list.push_back(curr_div_res.first.m_num[0]);
		rest = curr_div_res.second;
		if (0 == curr_idx)
		{
			break;
		}
		else
		{
			--curr_idx;
		}
	}

	rest.m_sign = m_sign != divisor.m_sign;

	// stripping high zeros
	while (quotient_list.size() > 0 && *quotient_list.begin() == 0)
	{
		quotient_list.pop_front();
	}

	if (quotient_list.size() == 0)
	{
		return div_res(rpa_int(0), rest);
	}

	// just convert quotient's reverse list of items in rpa_int object
	// be careful with signs!
	rpa_int quotient;
	quotient.realloc(quotient_list.size());
	size_t i = 0;
	for (std::list< base_t >::reverse_iterator it = quotient_list.rbegin(); it != quotient_list.rend(); ++it, ++i)
	{
		quotient.m_num[i] = *it;
	}
	if (1 == quotient.m_size && 0 == quotient.m_num[0])
	{
		quotient.m_sign = false;
	}
	else
	{
		quotient.m_sign = m_sign != divisor.m_sign;
	}

	return div_res(quotient, rest);
}

// ----------------------------------------------------------------------------------------------------------------------------------------
rpa_int::div_res rpa_int::div_by_subtr(rpa_int const& divisor) const
{
	rpa_int rest(*this);
	rpa_int quotient;

	while (rest >= divisor)
	{
		rest -= divisor;
		++quotient;
	}
	rest.m_sign = true;
	quotient.m_sign = m_sign != divisor.m_sign;

	return div_res(quotient, rest);
}

// ----------------------------------------------------------------------------------------------------------------------------------------
rpa_int	rpa_int::operator / (long int num) const
{
	if (0 == num)
	{
#ifdef DISABLE_EXCEPTIONS
		return rpa_int();
#else
		throw RPA_INT_DIV_BY_ZERO;
#endif
	}
	else if (1 == num)
	{
		return *this;
	}
	else if (-1 == num)
	{
		rpa_int new_num(*this);
		new_num.m_sign = !new_num.m_sign;
		return new_num;
	}

	return *this / rpa_int(num);
}

// ----------------------------------------------------------------------------------------------------------------------------------------
void rpa_int::get_complement_masks(base_t& mask, base_t& spec_mask, size_t& spec_idx) const
{
	if (!m_sign)
	{
		mask = 0;
		spec_mask = 0;
		spec_idx = 0;
		return;
	}

	mask = s_BASE_T_MAX;
	spec_mask = s_BASE_T_MAX;
	spec_idx = 0;

	size_t last_one_idx = 1;
	base_t item = 0;
	for (size_t i = 0; i < m_size; ++i)
	{
		item = m_num[i];
		if (item != 0)
		{
			spec_idx = i;
			do
			{
				if (item & 0x1)
				{
					break;
				}
				else
				{
					item >>= 1;
					++last_one_idx;
				}
			} while (item != 0);
			break;
		}
	}
	spec_mask >>= last_one_idx;
	spec_mask <<= last_one_idx;
}

// ----------------------------------------------------------------------------------------------------------------------------------------
rpa_int& rpa_int::operator ++ ()
{
	if (m_sign)
	{
		m_sign = false;
		operator -- ();
		// the result of ++(-1) is +0.
		// so we don't need to setup sign-falg
		if (1 != m_size || 0 != m_num[0])
		{
			m_sign = true;
		}
	
		return *this;
	}

	if (m_num[0] != s_BASE_T_MAX)
	{
		++m_num[0];
	}
	else
	{
		m_num[0] = 0;
		base_t carry = 1;
		base_t next_carry = 0;
		for (size_t i = 1; i < m_size; ++i)
		{
			if (m_num[i] > s_BASE_T_MAX - carry)
			{
				next_carry = 1;
			}
			else
			{
				next_carry = 0;
			}
			m_num[i] += carry;
			carry = next_carry;
			if (0 == carry)
			{
				break;
			}
		}
		if (carry)
		{
			++m_size;
			base_t* reallocated_num = new base_t[m_size];
			memcpy(reallocated_num, m_num, sizeof(base_t) * (m_size - 1));
			delete[] m_num;
			m_num = reallocated_num;
			m_num[m_size - 1] = 1;
		}
	}

	return *this;
}

// ----------------------------------------------------------------------------------------------------------------------------------------
rpa_int rpa_int::operator ++ (int)
{
	rpa_int ret_num(*this);
	operator ++ ();

	return ret_num;
}

// ----------------------------------------------------------------------------------------------------------------------------------------
rpa_int& rpa_int::operator -- ()
{
	if (m_sign)
	{
		m_sign = false;
		operator ++ ();
		if (1 != m_size || 0 != m_num[0])
		{
			m_sign = true;
		}
	
		return *this;
	}

	if (m_num[0] > 0)
	{
		--m_num[0];
	}
	else
	{
		if (1 == m_size)
		{
			// Our number is zero. So the result is -1.
			m_num[0] = 1;
			m_sign = true;
		}
		else
		{
			base_t prev_item_value = 0;
			for (size_t i = 0; i < m_size; ++i)
			{
				prev_item_value = m_num[i];
				--m_num[i];
				if (prev_item_value > m_num[i])
				{
					break;
				}
			}
			if (0 == m_num[m_size - 1])
			{
				resize(m_size -1);
			}
		}
	}

	return *this;
}

// ----------------------------------------------------------------------------------------------------------------------------------------
rpa_int rpa_int::operator -- (int)
{
	rpa_int ret_num(*this);
	operator -- ();

	return ret_num;
}

// ----------------------------------------------------------------------------------------------------------------------------------------
bool operator == (long int i, rpa_int const& obj) { return obj == i; }
bool operator != (long int i, rpa_int const& obj) { return obj != i; }
bool operator > (long int i, rpa_int const& obj) { return obj < i; }
bool operator >= (long int i, rpa_int const& obj) { return obj <= i; }
bool operator < (long int i, rpa_int const& obj) { return obj > i; }
bool operator <= (long int i, rpa_int const& obj) { return obj >= i; }

rpa_int operator + (long int i, rpa_int const& obj) { return obj + i; }
rpa_int operator - (long int i, rpa_int const& obj) { return obj * (-1) + i; }
rpa_int operator * (long int i, rpa_int const& obj) { return obj * i; }
rpa_int operator / (long int i, rpa_int const& obj) { return rpa_int(i) / obj; }
rpa_int operator % (long int i, rpa_int const& obj) { return rpa_int(i) % obj; }

// ----------------------------------------------------------------------------------------------------------------------------------------
rpa_int rpa_int::abs() const
{
	if (false == m_sign)
	{
		return *this;
	}

	rpa_int ret_value(*this);
	ret_value.m_sign = false;

	return ret_value;
}

// ----------------------------------------------------------------------------------------------------------------------------------------
rpa_int rpa_int::gcd(rpa_int const& obj) const
{
	if (abs() == 1 || obj.abs() == 1)
	{
		return rpa_int(1);
	}
	else if (*this == obj)
	{
		return *this;
	}
	else if (*this == 0 || obj == 0)
	{
		return rpa_int(0);
	}

	rpa_int a(*this);
	rpa_int b(obj);

	// Euclid's algorithm, of course!

#if 0
	// It is rather slow!

	while (a != b)
	{
		if (a > b)
		{
			a -= b;
		}
		else
		{
			b -= a;
		}
	}
#else
	rpa_int c;
	while (b != 0)
	{
		c = a % b;
		a = b;
		b = c;
	}
#endif

	return a.abs();
}

// ----------------------------------------------------------------------------------------------------------------------------------------
long int rpa_int::get_int() const
{
#ifndef DISABLE_EXCEPTIONS
	if (m_size * s_BASE_T_SIZE > s_LONGINT_SIZE)
	{
		throw RPA_INT_INCORRECT_TYPE_CONV;
	}
#endif
	long int bit_mask;
	if (m_size * s_BASE_T_SIZE == s_LONGINT_SIZE)
	{
		// In this case we are not able to get
		// correct bit mask using LOW_BITS_MASK macro.
		bit_mask = LONG_MAX;
	}
	else
	{
		LOW_BITS_MASK(bit_mask, m_size * s_BASE_T_SIZE);
	}

	if (m_sign)
	{
		return -1 * ((*(long int*)m_num) & bit_mask);
	}

	return (*(long int*)m_num) & bit_mask;
}

