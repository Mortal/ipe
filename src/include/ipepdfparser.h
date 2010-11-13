// -*- C++ -*-
// --------------------------------------------------------------------
// PDF file parser
// --------------------------------------------------------------------
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (C) 1993-2010  Otfried Cheong

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

#ifndef IPEPDFPARSER_H
#define IPEPDFPARSER_H

#include "ipebase.h"

// --------------------------------------------------------------------

namespace ipe {

  class PdfNull;
  class PdfBool;
  class PdfNumber;
  class PdfString;
  class PdfName;
  class PdfRef;
  class PdfArray;
  class PdfDict;

  class PdfFile;

  class PdfObj {
  public:
    virtual ~PdfObj() = 0;
    virtual const PdfNull *null() const;
    virtual const PdfBool *boolean() const;
    virtual const PdfNumber *number() const;
    virtual const PdfString *string() const;
    virtual const PdfName *name() const;
    virtual const PdfRef *ref() const;
    virtual const PdfArray *array() const;
    virtual const PdfDict *dict() const;
    virtual void write(Stream &stream) const = 0;
    String repr() const;
  };

  class PdfNull : public PdfObj {
  public:
    explicit PdfNull() { /* nothing */ }
    virtual const PdfNull *null() const;
    virtual void write(Stream &stream) const;
  };

  class PdfBool : public PdfObj {
  public:
    explicit PdfBool(bool val) : iValue(val) { /* nothing */ }
    virtual const PdfBool *boolean() const;
    virtual void write(Stream &stream) const;
    inline bool value() const { return iValue; }
  private:
    bool iValue;
  };

  class PdfNumber : public PdfObj {
  public:
    explicit PdfNumber(double val) : iValue(val) { /* nothing */ }
    virtual const PdfNumber *number() const;
    virtual void write(Stream &stream) const;
    inline double value() const { return iValue; }
  private:
    double iValue;
  };

  class PdfString : public PdfObj {
  public:
    explicit PdfString(const String &val) : iValue(val) { /* nothing */ }
    virtual const PdfString *string() const;
    virtual void write(Stream &stream) const;
    inline String value() const { return iValue; }
  private:
    String iValue;
  };

  class PdfName : public PdfObj {
  public:
    explicit PdfName(const String &val) : iValue(val) { /* nothing */ }
    virtual const PdfName *name() const;
    virtual void write(Stream &stream) const;
    inline String value() const { return iValue; }
  private:
    String iValue;
  };

  class PdfRef : public PdfObj {
  public:
  explicit PdfRef(int val) : iValue(val) { /* nothing */ }
    virtual const PdfRef *ref() const;
    virtual void write(Stream &stream) const;
    inline int value() const { return iValue; }
  private:
    int iValue;
  };

  class PdfArray : public PdfObj {
  public:
    explicit PdfArray() { /* nothing */ }
    ~PdfArray();
    virtual const PdfArray *array() const;
    virtual void write(Stream &stream) const;
    void append(const PdfObj *);
    inline int count() const { return iObjects.size(); }
    const PdfObj *obj(int index, const PdfFile *file) const;
  private:
    std::vector<const PdfObj *> iObjects;
  };

  class PdfDict : public PdfObj {
  public:
    explicit PdfDict() { /* nothing */ }
    ~PdfDict();
    virtual const PdfDict *dict() const;
    virtual void write(Stream &stream) const;
    void setStream(const Buffer &stream);
    void add(String key, const PdfObj *obj);
    const PdfObj *get(String key, const PdfFile *file) const;
    inline int count() const { return iItems.size(); }
    inline String key(int index) const { return iItems[index].iKey; }
    inline Buffer stream() const { return iStream; }
    bool deflated() const;
    // Buffer Inflate() const;
  private:
    struct Item {
      String iKey;
      const PdfObj *iVal;
    };
    std::vector<Item> iItems;
    Buffer iStream;
  };

  // --------------------------------------------------------------------

  //! A PDF lexical token.
  struct PdfToken {
    //! The various types of tokens.
    enum TToken { EErr, EOp, EName, ENumber, EString, ETrue, EFalse, ENull,
		  EArrayBg, EArrayEnd, EDictBg, EDictEnd };
    //! The type of this token.
    TToken iType;
    //! The string representing this token.
    String iString;
  };

  class PdfParser {
  public:
    PdfParser(DataSource &source);

    inline void getChar() { iCh = iSource.getChar(); ++iPos; }
    inline bool eos() const { return (iCh == EOF); }
    inline PdfToken token() const { return iTok; }

    void getToken();
    PdfObj *getObject();
    PdfObj *getObjectDef();
    PdfDict *getTrailer();
    void skipXRef();

  private:
    void skipWhiteSpace();
    PdfArray *makeArray();
    PdfDict *makeDict();

  private:
    DataSource &iSource;
    int iPos;
    int iCh;
    PdfToken iTok;
  };

  class PdfFile {
  public:
    PdfFile();
    ~PdfFile();
    bool parse(DataSource &source);
    const PdfObj *object(int num) const;
    const PdfDict *catalog() const;
    const PdfDict *page() const;
  private:
    std::map<int, const PdfObj *> iObjects;
    const PdfDict *iTrailer;
  };

} // namespace

// --------------------------------------------------------------------
#endif
