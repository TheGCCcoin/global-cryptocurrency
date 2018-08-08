/*
Title: Dash v12.3 / Bitcoin Core / Blockchain LiveLogging++ dump

MIT License

Copyright (c) 2018 Web3 Crypto Wallet Team

 info@web3cryptowallet.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */

#ifndef __LLOG_DUMP__
#define __LLOG_DUMP__

#include <string>

class CBlock;
class CBlockIndex;
class CBlockHeader;

// low level api
void llogBegin(std::wstring path);
void llogEnd();
void llogPut(std::wstring msg);

// high level api
void llogLogReplace(std::wstring path, std::wstring msg, bool replace);
void llogLog(std::wstring path, std::wstring msg);
void llogReplace(std::wstring path, std::wstring msg);

// common
void llogLog(std::wstring path, std::wstring msg, std::wstring s, bool replace=false);
void llogLog(std::wstring path, std::wstring msg, std::string s, bool replace=false);
void llogLog(std::wstring path, std::wstring msg, const char *s, bool replace=false);

// value
void llogLog(std::wstring path, std::wstring msg, int i, bool replace=false);

// blockchain
void llogLog(std::wstring path, std::wstring msg, const CBlock &block);
void llogLog(std::wstring path, std::wstring msg, const CBlockIndex &blockinfo);
void llogLog(std::wstring path, std::wstring msg, const CBlockHeader &blockhdr);

// util memory
void llogLog(std::wstring path, std::wstring msg, const void *p, int size, int format);

#endif
