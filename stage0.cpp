//Reese Montgomery
//CS4301
//Stage 0

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <fstream>
#include <ctime>
#include <sstream>
#include <ctype.h>
#include <cstdlib>

using namespace std;

const int MAX_SYMBOL_TABLE_SIZE = 256;
enum storeType {INTEGER , BOOLEAN, PROG_NAME};
enum allocation {YES,NO};
enum modes {VARIABLE, CONSTANT};
string keyword[10] = {"program", "const", "var", "integer", "boolean", "begin", "end", "true", "false", "not"};

struct entry  //define symbol table entry format
{
    string internalName;
    string externalName;
    storeType dataType;
    modes mode;
    string value; 
    allocation alloc;
    int units;
};
vector<entry> symbolTable;
ifstream sourceFile;
ofstream listingFile,objectFile; 
string token;
char charac;
bool newLine = false;
string tempNewLine = "";
int numErrors = 0;
int lineNumber = 1;
int numBools = -1;
int numInts = -1;
const char END_OF_FILE = '$'; // arbitrary choice
time_t now = time (NULL);

void CreateListingHeader();
void Parser();
void CreateListingTrailer();
void PrintSymbolTable();
char NextChar();
string NextToken();
void Prog();
void ProgStmt();
void Consts();
void Vars();
void BeginEndStmt();
void ConstStmts();
void VarStmts();
string Ids();
void Insert(string, storeType, modes, string, allocation, int);
string GenInternalName(storeType);
storeType WhichType(string);
string WhichValue(string);
bool isDefined(string);
bool isKeyword(string str) //Checks to see if a given string is a reserved keyword
{
    for (unsigned i = 0; i < (sizeof(keyword)/sizeof(keyword[0])) - 1; i++)
    {
        if (str == keyword[i])
        {
            return true;
        }
    }
    return false;
}


int main(int argc, char **argv)
{
    //this program is the stage0 compiler for Pascallite.  It will accept
    //input from argv[1], generating a listing to argv[2], and object code to
    //argv[3]
    if (argc != 4)
    {
        cerr << "Usage: " << argv[0] << " dataFile listingFile objectFile" << endl;
        

        return 1;
    }
    

    sourceFile.open(argv[1]);
    listingFile.open(argv[2]);
    objectFile.open(argv[3]);
    
    CreateListingHeader();

    Parser();

    CreateListingTrailer();

    PrintSymbolTable();

    return 0;
}

void CreateListingHeader()
{
    /*  print "STAGE0:", names, DATE, TIME OF DAY;
        print "Line NO:", "SOURCE STATEMENT";
        //line numbers and source statements should be aligned under the headings */
    listingFile << setw(28) << left << "STAGE0:  Reese Montgomery";
    listingFile << ctime(&now) << endl;
    
    listingFile << "LINE NO.              SOURCE STATEMENT" << endl << endl;
    listingFile << setw(5) << right << lineNumber <<"|";

    lineNumber += 1;
    
}

void Parser()
{
    NextChar();
        //Charac must be initialized to the first character of the source file
    if (NextToken() != "program")
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": keyword \"program\" expected.";
        CreateListingTrailer();
        exit(1);
    }
        
    Prog();
    //parser implements the grammar rules, calling first rule */
}

void CreateListingTrailer()
{
    /* print "COMPILATION TERMINATED", "# ERRORS ENCOUNTERED"; */
    listingFile << endl << endl << "COMPILATION TERMINATED      " << numErrors << " ERRORS ENCOUNTERED" << endl;
}

void PrintSymbolTable()
{
    objectFile << setw(28) << left << "STAGE0:  Reese Montgomery"; //Print header
    objectFile << ctime(&now) << endl;
    objectFile << "Symbol Table" << endl << endl;

    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        //print names
        objectFile << setw(15) << left << symbolTable[i].externalName << "  ";
        objectFile << setw(4) << left << symbolTable[i].internalName << "  ";

        //print dataType
        if (symbolTable[i].dataType == PROG_NAME)
        {
            objectFile << setw(9) << right << "PROG_NAME" << "  ";
        }
        else if (symbolTable[i].dataType == INTEGER)
        {
            objectFile << setw(9) << right << "INTEGER" << "  ";
        }
        else if (symbolTable[i].dataType == BOOLEAN)
        {
            objectFile << setw(9) << right << "BOOLEAN" << "  ";
        }

        //print mode
        if (symbolTable[i].mode == CONSTANT)
        {
            objectFile << setw(8) << right << "CONSTANT" << "  ";
        }
        else if (symbolTable[i].mode == VARIABLE)
        {
            objectFile << setw(8) << right << "VARIABLE" << "  ";
        }

        //print value
        objectFile << setw(15) << right << symbolTable[i].value << " ";

        //print alloc
        if (symbolTable[i].alloc == YES)
        {
            objectFile << setw(4) << right << "YES" << "  ";
        }
        else if (symbolTable[i].alloc == NO)
        {
            objectFile << setw(4) << right << "NO" << "  ";
        }

        //print # of units
        objectFile << symbolTable[i].units << endl;
    }
}

void Prog()
{
    //token should be "program"
    if (token != "program")
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": keyword \"program\" expected.";
        CreateListingTrailer();
        exit(1);
    }
    ProgStmt();
    if (token == "const") Consts();
    if (token == "var") Vars();
    if (token != "begin") 
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": keyword \"begin\" expected.";
        CreateListingTrailer();
        exit(1);
    }    
    BeginEndStmt();
    if (NextToken()[0] != (END_OF_FILE))
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": no text may follow \"end\"";
        CreateListingTrailer();
        exit(1);
    }
}

void ProgStmt() //token should be "program"
{
    string x;
    if (token != "program")
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": keyword \"program\" expected.";
        CreateListingTrailer();
        exit(1);
    }

    x = NextToken();

    if (isKeyword(token) || token == ";") //check for program name
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": program name expected";
        CreateListingTrailer();
        exit(1);
    }

    if (NextToken() != ";") //make sure program name is followed by ";"
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": semicolon expected.";
        CreateListingTrailer();
        exit(1);
    }
    NextToken();
    Insert(x, PROG_NAME, CONSTANT, x, NO, 0); //insert prog name into symbol table
}

void Consts() //token should be "const"
{
    if (token != "const")
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": keyword \"const\" expected.";
        CreateListingTrailer();
        exit(1);
    }

    NextToken();
    if (isKeyword(token)) //make sure token is not a reserved keyword
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": non-keyword identifier must follow \"const\"";
        CreateListingTrailer();
        exit(1);
    }
    ConstStmts(); //process const stmts
}

void Vars() //token should be "var"
{
    if (token != "var")
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": keyword \"var\" expected.";
        CreateListingTrailer();
        exit(1);
    }

    if (isKeyword(NextToken())) //make sure token following var is not a reserved keyword
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": non-keyword identifier must follow \"var\"";
        CreateListingTrailer();
        exit(1);
    }
    VarStmts(); //process var stmts
}

void BeginEndStmt() // token should be "begin"
{
    if (token != "begin")
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": keyword \"begin\" expected.";
        CreateListingTrailer();
        exit(1);
    }

    if (NextToken() != "end") //make sure "end" follows "begin"
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": keyword \"end\" expected.";
        CreateListingTrailer();
        exit(1);
    }
    if (NextToken() != ".") //make sure "end" is followed by a period
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": \".\" expected.";
        CreateListingTrailer();
        exit(1);
    }
    /*  */
}

void ConstStmts() //token should now be NON_KEY_ID
{
    string x, y;
    if (isKeyword(token)) //make sure token is not a keyword
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": non-keyword identifier expected";
        CreateListingTrailer();
        exit(1);
    }

    x = token; //name of const
    if (NextToken() != "=") // make sure non key id is followed by an equals sign
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": \"=\" expected";
        CreateListingTrailer();
        exit(1);
    }

    y = NextToken(); //value of x    

    if (y == "+" || y == "-") //if token begins with a sign, make sure all characters following the sign are digits
    {
        NextToken();
        string z = token;

        stringstream is(z);
        int i;

        is >> i;
        if (i)
        {
            y = y + token;
        }
        else 
        {
            numErrors += 1;
            listingFile << endl << "Error: Line " << lineNumber - 1 <<": integer expected after sign";
            CreateListingTrailer();
            exit(1);
        }

    }

    if (y == "not") //if token is not, make sure it is followed by true or false
    {
        if (NextToken() != "true" && token != "false")
        {
            numErrors += 1;
            listingFile << endl << "Error: Line " << lineNumber - 1 <<": boolean expected after not";
            CreateListingTrailer();
            exit(1);
        }

        if (token == "true") 
            y = "false";
        else
            y = "true";
    }

    bool isDefined = false;

    for (unsigned i = 0; i < symbolTable.size(); i++) //check if token has already been defined
    {
        if (y.substr(0,15) == symbolTable[i].externalName)
        {
            isDefined = true;
        }
    }
    
    if (!isDefined)
    {
        if (!isKeyword(y) && y.find_first_not_of("+-0123456789") != string::npos)
        {
            numErrors += 1;
            listingFile << endl << "Error: Line " << lineNumber - 1 <<": reference to undefined constant";
            CreateListingTrailer();
            exit(1);
        }
    }

    if (NextToken() != ";") //make sure each literal is followed by a ";"
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": \";\" expected";
        CreateListingTrailer();
        exit(1);
    }
    
    Insert(x.substr(0, 15), WhichType(y), CONSTANT, WhichValue(y), YES, 1); //insert value into symbol table

    if (!isKeyword(NextToken())) //if next token is non key id, run conststmts again
        ConstStmts();
        
    if (token != "var" && token != "begin") // if it is a keyword, but is not begin or var, exit
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": non-keyword identifier,\"begin\", or \"var\" expected";
        CreateListingTrailer();
        exit(1);
    }

    
}

void VarStmts() // token should be NON_KEY_ID
{
    string x,y;
    if (isKeyword(token)) //make sure token is not a reserved keyword
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": non-keyword identifier expected";
        CreateListingTrailer();
        exit(1);
    }

    x = Ids(); //gets all var names if it is a list

    if (token != ":") // make sure token following non-key ids is a colon
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": \":\" expected";
        CreateListingTrailer();
        exit(1);
    }

    if (NextToken() != "integer" && token != "boolean") //token following colon must be a data type
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": illegal type follows \":\"";
        CreateListingTrailer();
        exit(1);
    }

    y = token; //save data type

    if(NextToken() != ";") //make sure data type is followed by semicolon
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": semicolon expected";
        CreateListingTrailer();
        exit(1);
    }

    if (y == "integer") //insert integer values into table
    {
        Insert(x, INTEGER, VARIABLE, "", YES, 1);
    }
    else if (y == "boolean") //insert boolean values into table
    {
        Insert(x, BOOLEAN, VARIABLE, "", YES, 1);
    }

    if (!isKeyword(NextToken())) //if followed by non key id, run varstmts again
    {
        VarStmts();
    }
    if (token != "begin") //if it is a keyword, make sure it is "begin"
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": non-keyword identifier or \"begin\" expected";
        CreateListingTrailer();
        exit(1);
    }
    
}

string Ids() //puts all non key ids from a list into one string to be inserted into table
{
    string temp, tempString;
    if (isKeyword(token)) //make sure token is not reserved keyword
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": non-keyword identifier expected";
        CreateListingTrailer();
        exit(1);
    }

    tempString = token;
    temp = token;

    if (NextToken() == ",") //look for commas separating non key ids
    {
        if(isKeyword(NextToken())) //make sure following token isnt a keyword
        {
            numErrors += 1;
            listingFile << endl << "Error: Line " << lineNumber - 1 <<": non-keyword identifier expected";
            CreateListingTrailer();
            exit(1);
        }
        tempString = temp + "," + Ids(); //attach non key id to string
    }
    return tempString;
}

void Insert(string externalName, storeType inType, modes inMode, string inValue,
            allocation inAlloc, int inUnits)
    //create symbol table entry for each identifier in list of external names
    //multiply inserted names are illegal
{
    
    string name;

    while (externalName.length() != 0) //this while loop extracts each non key id from a string delimited by commas
    {
        name = "";
        for (unsigned i = 0; i < externalName.length(); i++)
        {
            if (externalName[i] == ',') // look for comma in string, if found, break out of for loop
            {
                break;
            }
            name += externalName[i];
        }

        if(name.length() == externalName.length()) // if name length == external name length, we've reached the end of the list
        {
            externalName = "";
        }
        else
        {
            externalName = externalName.substr(name.length() + 1, externalName.length() - name.length()); //if not, get rid of the part of external name we just copied into name
        }

        for (unsigned i = 0; i < symbolTable.size(); i++) //loop through all current entries of symbol table
        {
            if(symbolTable[i].externalName == name.substr(0, 15)) //check if name has alread been defined in symbolTable
            {
                numErrors += 1;
                listingFile << endl << "Error: Line " << lineNumber - 1 <<": " << name.substr(0,15) << " is multiply defined";
                CreateListingTrailer();
                exit(1);
            } 
        }

        if(isKeyword(name)) //check if name is a reserved keyword
        {
            numErrors += 1;
            listingFile << endl << "Error: Line " << lineNumber - 1 <<": illegal use of keyword";
            CreateListingTrailer();
            exit(1);
        }
        else //create table entry
        {
            entry newEntry = {GenInternalName(inType), name.substr(0, 15) , inType, inMode, inValue.substr(0,15), inAlloc, inUnits};
            if (symbolTable.size() == MAX_SYMBOL_TABLE_SIZE)
            {
                numErrors += 1;
                listingFile << endl << "Error: Line " << lineNumber - 1 <<": Symbol table overflow";
                CreateListingTrailer();
                exit(1);
            }
            symbolTable.push_back(newEntry);
        }
    }
}

string GenInternalName(storeType inType) // this aptly named function generates the internal name of a symbol for the symbol table
{
    stringstream s;

    if (inType == PROG_NAME)
    {
        return "P0";
    }
    else if (inType == INTEGER)
    {
        numInts += 1;
        s << "I" << numInts;
        return s.str();
    }
    else
    {
        numBools += 1;
        s << "B" << numBools;
        return s.str();
    }
}

storeType WhichType(string name) //this function returns whether the value of the string is a boolean or an integer value
{
    istringstream inputString(name);
    int i;
    string s;

    inputString >> i;
    if (inputString)
    {
        return INTEGER;
    }

    inputString.clear();
    inputString.str(name);

    inputString >> s;
    if (s == "true" || s == "false" || s == "not")
    {
        return BOOLEAN;
    }

    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if(symbolTable[i].externalName == name) //check if name has alread been defined in symbolTable
        {
            return symbolTable[i].dataType;
        } 
    }

    numErrors += 1;
    listingFile << endl << "Error: Line " << lineNumber - 1 <<": reference to undefined constant";
    CreateListingTrailer();
    exit(1);

}

string WhichValue(string name) // this function returns the value of the symbol for the symbol table
{
    if (name == "true" || name == "false")
    {
        return (name == "true") ? "1" : "0"; //if name is true, return 1, if false, return 0
    }

    if (name == "not true" || name == "not false")
    {
        return (name == "not true") ? "0" : "1"; //if name is not true, return 0, if not false, return 1
    }

    istringstream inputString(name);
    int i;

    inputString >> i;
    if (inputString)
    {
        return name;
    }

    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if(symbolTable[i].externalName == name) //check if name has alread been defined in symbolTable
        {
            return symbolTable[i].value;
        } 
    }
    
    numErrors += 1;
    listingFile << endl << "Error: Line " << lineNumber - 1 <<": reference to undefined constant";
    CreateListingTrailer();
    exit(1);
}

string NextToken() ////returns the next token or end of file marker
{
    token = "";
    while (token == "")
    {
        switch(charac)
        {
            case '{'                : //process comment
                                    while (charac != END_OF_FILE && charac != '}')
                                    {
                                        NextChar();
                                    }
                                    if (charac == END_OF_FILE)
                                    {
                                        numErrors += 1;
                                        listingFile << endl << "Error: Line " << lineNumber - 1 <<": unexpected end of file";
                                        CreateListingTrailer();
                                        exit(1);
                                    }
                                    NextChar();
                                    break;

            case '}'                : numErrors += 1;
                                    listingFile << endl << "Error: Line " << lineNumber - 1 <<": '}' cannot begin token";
                                    CreateListingTrailer();
                                    exit(1);
                                    break;
            
            case ' ' : //ignore whitespace
            case '\n':
            case '\r':
            case '\t':
                                    NextChar();
                                    break;

            case ':': //special characters
            case ',':
            case ';':
            case '=':
            case '+':
            case '-':
            case '.':
                                    token += charac;
                                    NextChar();
                                    break;

            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
            case 'g':
            case 'h':
            case 'i':
            case 'j':
            case 'k':
            case 'l':
            case 'm': 
            case 'n': 
            case 'o': 
            case 'p':               
            case 'q': 
            case 'r': 
            case 's': 
            case 't': 
            case 'u': 
            case 'v': 
            case 'w': 
            case 'x': 
            case 'y': 
            case 'z': 
                                    token = charac;
                                    while (isalnum(NextChar()) || charac == '_')
                                    {
                                        token += charac;
                                    }
                                    if (token[token.length() - 1] == '_')
                                    {
                                        numErrors += 1;
                                        listingFile << endl << "Error: Line " << lineNumber - 1 <<": '_' cannot end token";
                                        CreateListingTrailer();
                                        exit(1);
                                    }
                                    break;

            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                                    token = charac;
                                    while (isdigit(NextChar())) token += charac;
                                    break;

            case END_OF_FILE        : token = charac;
                                    break;

            default                 : numErrors += 1;
                                    listingFile << endl << "Error: Line " << lineNumber - 1 <<": illegal symbol detected";
                                    CreateListingTrailer();
                                    exit(1);
                                    break;           
        }
    }
    return token;
}

char NextChar() // returns the next character or eof marker
{
    //char c;
    charac = sourceFile.get();
    if (!sourceFile)
    {
        charac = END_OF_FILE;
    }  
    
    if (charac == '\n' && newLine == false)
    {
        newLine = true;
        return charac;
    }  
    else if (charac == '\n' && newLine == true && !(tempNewLine.empty()))
    {
        listingFile << endl << setw(5) << lineNumber <<"|";
        listingFile << tempNewLine << "";
        lineNumber += 1;
        tempNewLine = "";
        return charac;
    }        
    else if (charac == '\n' && newLine == true)
    {
        listingFile << endl << setw(5) << lineNumber <<"|";
        lineNumber += 1;
        return charac;
    }
    
    if (isspace(charac) && newLine == true)
    {
        tempNewLine += charac;
        return charac;
    }
    else if (charac != ' ' && newLine == true && charac != END_OF_FILE)
    {
        listingFile << endl << setw(5) << lineNumber <<"|";
        listingFile << tempNewLine << "";
        listingFile << charac;
        lineNumber += 1;
        newLine = false;
        tempNewLine = "";
        return charac;
    }
    
    if (charac != END_OF_FILE && newLine == false)
    {
        listingFile << charac;
    }

    return charac;
}
