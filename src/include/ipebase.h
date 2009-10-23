// -*- C++ -*-
// --------------------------------------------------------------------
// Base header file --- must be included by all Ipe components.
// --------------------------------------------------------------------
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (C) 1993-2009  Otfried Cheong

    Ipe is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    As a special exception, you have permission to link Ipe with the
    CGAL library and distribute executables, as long as you follow the
    requirements of the Gnu General Public License in regard to all of
    the software in the executable aside from CGAL.

    Ipe is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
    License for more details.

    You should have received a copy of the GNU General Public License
    along with Ipe; if not, you can find it at
    "http://www.gnu.org/copyleft/gpl.html", or write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifndef IPEBASE_H
#define IPEBASE_H

#include <cstdio>
#include <cstring>
#include <vector>
#include <map>
#include <list>
#include <algorithm>

// --------------------------------------------------------------------

typedef unsigned char uchar;

#if defined(__MINGW32__) || defined(__APPLE__)
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long int ulong;
#endif

#undef assert
#define assert(e) ((e) ? (void)0 : ipeAssertionFailed(__FILE__, __LINE__, #e))
extern void ipeAssertionFailed(const char *, int, const char *);

// --------------------------------------------------------------------

extern void ipeDebug(const char *msg, ...);

template<class T>
class IpeAutoPtr {
public:
  IpeAutoPtr(T *ptr) : iPtr(ptr) { /* nothing */ }
  ~IpeAutoPtr() { delete iPtr; }
  T *operator->() { return iPtr; }
  T &operator*() { return *iPtr; }
  T *ptr() { return iPtr; }
  T *take() { T *p = iPtr; iPtr = 0; return p; }
private:
  T *iPtr;
};

namespace ipe {

  //! Ipelib version.
  /*! \ingroup base */
  const int IPELIB_VERSION = 70007;

  //! Oldest readable file format version.
  /*! \ingroup base */
  const int OLDEST_FILE_FORMAT = 70000;
  //! Current file format version.
  /*! \ingroup base */
  const int FILE_FORMAT = 70005;

  // --------------------------------------------------------------------

  class Stream;

  class String {
  public:
    String();
    String(const char *str);
    String(const String &rhs);
    String(const String &rhs, int index, int len);
    String &operator=(const String &rhs);
    ~String();
    //! Return character at index i.
    inline char operator[](int i) const { return iImp->iData[i]; }
    //! Is the string empty?
    inline bool empty() const { return (size() == 0); }
    //! Is the string empty?
    inline bool isEmpty() const { return (size() == 0); }
    //! Return read-only pointer to the data.
    const char *data() const { return iImp->iData; }
    //! Return number of bytes in the string.
    inline int size() const { return iImp->iSize; }
    //! Operator syntax for append.
    inline void operator+=(const String &rhs) { append(rhs); }
    //! Operator syntax for append.
    void operator+=(char ch) { append(ch); }
    //! Create substring.
    inline String substr(int i, int len = -1) const {
      return String(*this, i, len); }
    //! Create substring at the left.
    inline String left(int i) const {
      return String(*this, 0, i); }
    String right(int i) const;
    //! Operator !=.
    inline bool operator!=(const String &rhs) const {
      return !(*this == rhs); }

    int find(char ch) const;
    int rfind(char ch) const;
    int find(const char *rhs) const;
    void erase();
    void append(const String &rhs);
    void append(char ch);
    bool operator==(const String &rhs) const;
    bool operator<(const String &rhs) const;
    String operator+(const String &rhs) const;
    int unicode(int &index) const;
    const char *z() const;
  private:
    void detach(int n);
  private:
    struct Imp {
      int iRefCount;
      int iSize;
      int iCapacity;
      char *iData;
    };
    Imp *iImp;
  };

  // --------------------------------------------------------------------

  class Fixed {
  public:
    explicit Fixed(int val) : iValue(val * 1000) { /* nothing */ }
    explicit Fixed() { /* nothing */ }
    inline static Fixed fromInternal(int val);
    static Fixed fromDouble(double val);
    inline int toInt() const { return iValue / 1000; }
    inline double toDouble() const { return iValue / 1000.0; }
    inline int internal() const { return iValue; }
    Fixed mult(int a, int b) const;
    inline bool operator==(const Fixed &rhs) const;
    inline bool operator!=(const Fixed &rhs) const;
    inline bool operator<(const Fixed &rhs) const;
  private:
    int iValue;

    friend Stream &operator<<(Stream &stream, const Fixed &f);
  };

  // --------------------------------------------------------------------

  class Lex {
  public:
    explicit Lex(String str);

    String token();
    String nextToken();
    int getInt();
    int getHexByte();
    Fixed getFixed();
    unsigned long int getHexNumber();
    double getDouble();
    //! Extract next character (not skipping anything).
    inline char getChar() {
      return iString[iPos++]; }
    void skipWhitespace();

    //! Operator syntax for getInt().
    inline Lex &operator>>(int &i) {
      i = getInt(); return *this; }
    //! Operator syntax for getDouble().
    inline Lex &operator>>(double &d) {
      d = getDouble(); return *this; }

    //! Operator syntax for getFixed().
    inline Lex &operator>>(Fixed &d) {
      d = getFixed(); return *this; }

    //! Mark the current position.
    inline void mark() {
      iMark = iPos; }
    //! Reset reader to the marked position.
    inline void fromMark() {
      iPos = iMark; }

    //! Return true if at end of string (not even whitespace left).
    inline bool eos() const {
      return (iPos == iString.size()); }

  private:
    String iString;
    int iPos;
    int iMark;
  };

  // --------------------------------------------------------------------

  class Buffer {
  public:
    Buffer();
    ~Buffer();
    Buffer(const Buffer &rhs);
    Buffer &operator=(const Buffer &rhs);

    explicit Buffer(int size);
    explicit Buffer(const char *data, int size);
    //! Character access.
    inline char &operator[](int index) { return iImp->iData[index]; }
    //! Character access (const version).
    inline const char &operator[](int index) const
    { return iImp->iData[index]; }
    //! Return size of buffer;
    inline int size() const { return iImp->iSize; }
    //! Return pointer to buffer data.
    inline char *data() { return iImp->iData; }
    //! Return pointer to buffer data (const version).
    inline const char *data() const { return iImp->iData; }

  private:
    struct Imp {
      int iRefCount;
      char *iData;
      int iSize;
    };
    Imp *iImp;
  };

  // --------------------------------------------------------------------

  class Stream {
  public:
    //! Virtual destructor.
    virtual ~Stream();
    //! Output character.
    virtual void putChar(char ch) = 0;
    //! Close the stream.  No more writing allowed!
    virtual void close();
    //! Output string.
    virtual void putString(String s);
    //! Output C string.
    virtual void putCString(const char *s);
  //! Output raw character data.
    virtual void putRaw(const char *data, int size);
    //! Output character.
    inline Stream &operator<<(char ch) { putChar(ch); return *this; }
    //! Output string.
    inline Stream &operator<<(const String &s) { putString(s); return *this; }
    //! Output C string.
    inline Stream &operator<<(const char *s) { putCString(s); return *this; }
    Stream &operator<<(int i);
    Stream &operator<<(double d);
    void putHexByte(char b);
    void putXmlString(String s);
  };

  /*! \class ipe::TellStream
    \ingroup base
    \brief Adds position feedback to IpeStream.
  */

  class TellStream : public Stream {
  public:
    virtual int tell() const = 0;
  };

  class StringStream : public TellStream {
  public:
    StringStream(String &string);
    virtual void putChar(char ch);
    virtual void putString(String s);
    virtual void putCString(const char *s);
    virtual void putRaw(const char *data, int size);
    virtual int tell() const;
  private:
    String &iString;
  };

  class FileStream : public TellStream {
  public:
    FileStream(std::FILE *file);
    virtual void putChar(char ch);
    virtual void putString(String s);
    virtual void putCString(const char *s);
    virtual void putRaw(const char *data, int size);
    virtual int tell() const;
  private:
    std::FILE *iFile;
  };

  // --------------------------------------------------------------------

  class DataSource {
  public:
    virtual ~DataSource() = 0;
    //! Get one more character, or EOF.
    virtual int getChar() = 0;
  };

  class FileSource : public DataSource {
  public:
    FileSource(std::FILE *file);
    virtual int getChar();
  private:
    std::FILE *iFile;
  };

  class BufferSource : public DataSource {
  public:
    BufferSource(const Buffer &buffer);
    virtual int getChar();
  private:
    const Buffer &iBuffer;
    int iPos;
  };

  // --------------------------------------------------------------------

  class Platform {
  public:
    typedef void (*DebugHandler)(const char *);

#ifdef WIN32
    static String ipeDir(const char *suffix);
#endif
    static int libVersion();
    static void initLib(int version);
    static void setDebug(bool debug);
    static char pathSeparator();
    static String currentDirectory();
    static String latexDirectory();
    static String fontmapFile();
    static bool fileExists(String fname);
    static String readFile(String fname);
    static int runPdfLatex(String dir);
  };

  // --------------------------------------------------------------------

  inline bool Fixed::operator==(const Fixed &rhs) const
  {
    return iValue == rhs.iValue;
  }

  inline bool Fixed::operator!=(const Fixed &rhs) const
  {
    return iValue != rhs.iValue;
  }

  inline bool Fixed::operator<(const Fixed &rhs) const
  {
    return iValue < rhs.iValue;
  }

  inline Fixed Fixed::fromInternal(int val)
  {
    Fixed f;
    f.iValue = val;
    return f;
  }


} // namespace

// --------------------------------------------------------------------
#endif
