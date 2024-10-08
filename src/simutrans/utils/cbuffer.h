/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef UTILS_CBUFFER_H
#define UTILS_CBUFFER_H

#include <stdarg.h>
#include <stdlib.h>

/**
 * A character buffer. Main operation is 'append'
 */
class cbuffer_t
{
private:

	size_t capacity;

	/**
	* Number of characters without(!) trailing '\0'
	*/
	size_t size;

	char *buf;

	/**
	 * Implementation for copy constructor and copy assignment operator
	 */
	void copy( const cbuffer_t& cbx );

	/**
	 * Implementation for destructor and copy assignment operator
	 */
	void free();

public:
	/**
	 * Number of characters without(!) trailing '\0'
	 */
	int len() const { return size; }

	/**
	 * Creates a new cbuffer with capacity cap
	 */
	cbuffer_t();

	/**
	 * Creates a new cbuffer with the given text
     * @param text The text to initialize the buffer 
	 */
	cbuffer_t(const char * text);

	~cbuffer_t();

	/**
	 * Copy constructor
	 */
	cbuffer_t(const cbuffer_t& cbx);

	/**
	 * Copy assignment operator
	 */
	cbuffer_t& operator= (const cbuffer_t& cbx);

	/**
	 * Clears the buffer
	 */
	void clear();


	/**
	 * Sets the buffer to the given text 
	 * @param text The new text
	 */
	void set(const char * text);


	/**
	 * Appends text. Buffer will be extended if it does not have enough capacity.
	 */
	void append(const char * text);


	/**
	 * Appends text, at most n characters worth. Buffer will be extended if needed.
	 * maxchars should NOT include the null at the end of the string!
	 *  (e.g. it should be equivalent to the output of strlen())
	 */
	void append(const char* text, size_t maxchars);


	/**
	 * Appends unsigned number. Buffer will be extended if it does not have enough capacity.
	 */
	void append(unsigned int n);


	/**
	 * Removes all trailing whitespaces
	 */
	void rtrim();


	/**
	 * Return contents of buffer
	 */
	const char* get_str() const;


	/**
	 * Appends a number. Buffer will be extended if it does not have enough capacity.
	 */
	void append(double n, int precision);


	/**
	 * appends formatted money string
	 */
	void append_money(double money);


	/*
	 * Append formatted text to the buffer 
	 */
	void printf(const char *fmt, ...);

	void vprintf(const char *fmt,  va_list args );

	/* enlarge the buffer if needed (i.e. size+by_amount larger than capacity) */
	void extend(unsigned int by_amount);

	// removes trailing whitespaces
	void trim();

	/**
	 * Automagic conversion to a const char* for backwards compatibility
	 */
	operator const char *() const {return buf;}

	/// checks whether format specifiers in @p translated match those in @p master
	static bool check_and_repair_format_strings(const char* master, const char* translated, char **repaired = NULL);
};

#endif
