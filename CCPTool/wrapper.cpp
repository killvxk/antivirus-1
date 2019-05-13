#include "wrapper.h"
using namespace std;

//const bool DEBUG = true;
const bool DEBUG = false;


//TODO: better exceptions
OneStepDisasm::OneStepDisasm(string filename, int mode, uint64_t startaddr, uint64_t v_addr)
: _filename(filename)
, _codefile(filename, ios::in|ios::binary|ios::ate) //opening the file in binary mode
, _startaddr(startaddr)
, _v_addr(v_addr)
, _mode(mode)
{
	if (!_codefile.is_open())
		throw runtime_error("Can't open file");


	//opening disassembler
	if (mode == 32)
		cs_open(CS_ARCH_X86, CS_MODE_32, &_handle);
	else if (mode == 64)
		cs_open(CS_ARCH_X86, CS_MODE_64, &_handle);
	else
		throw runtime_error("Unrecognized mode");


	//turning on additional instruction information
	cs_option(_handle, CS_OPT_DETAIL, CS_OPT_ON);

		
	//allocating memory cache
	_insn = cs_malloc(_handle);

	
	_codesize = static_cast<uint64_t>( _codefile.tellg() ) - startaddr; //a number of bytes from begin to end + 1 is exactly this diffrnce

	if (DEBUG)
	{
		cout << "allocating " << _codefile.tellg() << " - " << startaddr << " = " << _codesize << endl;
	}


	//making a smart pointer point to the new memory location, with custom deleter for arrays
	_code_begin.reset( new uint8_t[_codesize], std::default_delete<uint8_t[]>() );

	_code_current = _code_begin.get();


	//positioning at offset startaddr
	_codefile.seekg(startaddr, ios::beg);
	//and reading the file to memory
	//via a bit of casts, because read() expects a *char and capstone expects uint8_t
	auto codeBeginChar = const_cast<char*>(reinterpret_cast<const char*>(_code_current));
	_codefile.read(codeBeginChar, _codesize);
	_codefile.close();
}


OneStepDisasm::OneStepDisasm(const OneStepDisasm& r)
: _filename(r._filename)
, _codefile(r._filename, ios::in|ios::binary|ios::ate)
, _code_begin(r._code_begin)
, _mode(r._mode)
{
	if (!_codefile.is_open())
		throw runtime_error("Can't open file");


	//god damn C in big projects
	if (_mode == 32)
		cs_open(CS_ARCH_X86, CS_MODE_32, &_handle);
	else if (_mode == 64)
		cs_open(CS_ARCH_X86, CS_MODE_64, &_handle);
	else
		throw runtime_error("Unrecognized mode");
	
	
	//turning on additional instruction information
	cs_option(_handle, CS_OPT_DETAIL, CS_OPT_ON);

	
	//positioning at offset startaddr
	_codefile.seekg(_startaddr, ios::beg);

		
	//allocating memory cache
	_insn = cs_malloc(_handle);


	_codesize = r._codesize;
	_startaddr = r._startaddr;
	_code_current = r._code_current;
	//also copy lifetime and virtual address
	_lifetime = r._lifetime;
	_v_addr = r._v_addr;
}

	
OneStepDisasm::~OneStepDisasm()
{
	cs_free(_insn, 1);
}


OneStepDisasm::instruction::instruction(unsigned int cid, uint64_t caddress, const char* cmnemonic, const char* cop_str, cs_detail* details)
: mnemonic(cmnemonic)
, operands(cop_str)
, id(cid)
, address(caddress)
, empty(false)
, groups( details->groups, details->groups + details->groups_count ) //initialize a vector of groups from c array
{  }


OneStepDisasm::instruction::instruction(const instruction& r)
: mnemonic(r.mnemonic)
, operands(r.operands)
, id(r.id)
, address(r.address)
, empty(r.empty)
, groups(r.groups)
{  }


OneStepDisasm::instruction::instruction()
: mnemonic("i fuck you")
, operands("nigga bitch")
, address(0)
, id(0)
, empty("true")
, groups()
{  }



OneStepDisasm::instruction::instruction(bool cempty)
: mnemonic("empty insn")
, operands("empty insn")
, address(0)
, id(0)
, empty("true")
, groups()
{
	if (cempty != true)
		throw runtime_error("Trying to construct an empty instruction with cempty set to false");
}


OneStepDisasm::instruction OneStepDisasm::next()
{
	bool success = cs_disasm_iter(_handle, &_code_current, &_codesize, &_v_addr, _insn); //changed _startaddr to _v_addr
	if (!success)
	{
		OneStepDisasm::instruction t {true};
		return t;
	}
	OneStepDisasm::instruction t {_insn->id, _insn->address, _insn->mnemonic, _insn->op_str, _insn->detail};
	return t;
}

OneStepDisasm::instruction OneStepDisasm::current() const
{
	return OneStepDisasm::instruction (_insn->id, _insn->address, _insn->mnemonic, _insn->op_str, _insn->detail);
}

OneStepDisasm OneStepDisasm::clone_at(const uint64_t &addr)
{
	if (addr >= _v_addr)
	{
		//3 argument: must to check overflow, fix it later
		OneStepDisasm child (_filename, _mode, _startaddr + (addr - _v_addr), addr);
		return child;
	}
	else
	{
		//again need to add int overflow checking
		OneStepDisasm child(_filename, _mode, _startaddr - (_v_addr - addr), addr);
		return child;
	}
}

int OneStepDisasm::get_mode() const
{
	return _mode;
}
