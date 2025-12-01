// Standalone Script Compiler & VM
// Console application for testing the scripting system independently

#include "Platform.h"
#include "ScriptToken.h"
#include "ScriptAST.h"
#include "ScriptParser.h"
#include "ScriptCompiler.h"
#include "ScriptVM.h"
#include "ScriptBytecode.h"

#include <iostream>
#include <fstream>
#include <chrono>

// Simple lexer (tokenizer) - standalone version
class FScriptLexer
{
private:
    FString Source;
    int32 Start = 0;
    int32 Current = 0;
    int32 Line = 1;
    int32 Column = 1;
    TArray<FScriptToken> Tokens;
    
    bool IsAtEnd() const { return Current >= static_cast<int32>(Source.length()); }
    
    char Advance()
    {
        Column++;
        return Source[Current++];
    }
    
    char Peek() const
    {
        if (IsAtEnd()) return '\0';
        return Source[Current];
    }
    
    char PeekNext() const
    {
        if (Current + 1 >= static_cast<int32>(Source.length())) return '\0';
        return Source[Current + 1];
    }
    
    bool Match(char expected)
    {
        if (IsAtEnd()) return false;
        if (Source[Current] != expected) return false;
        Current++;
        Column++;
        return true;
    }
    
    void AddToken(ETokenType type)
    {
        FString text = Source.substr(Start, Current - Start);
        Tokens.push_back(FScriptToken(type, text, Line, Column - static_cast<int32>(text.length())));
    }
    
    void AddToken(ETokenType type, const FString& lexeme)
    {
        Tokens.push_back(FScriptToken(type, lexeme, Line, Column - static_cast<int32>(lexeme.length())));
    }
    
    void ScanToken()
    {
        char c = Advance();
        
        switch (c)
        {
            case '(': AddToken(ETokenType::LEFT_PAREN); break;
            case ')': AddToken(ETokenType::RIGHT_PAREN); break;
            case '{': AddToken(ETokenType::LEFT_BRACE); break;
            case '}': AddToken(ETokenType::RIGHT_BRACE); break;
            case '[': AddToken(ETokenType::LEFT_BRACKET); break;
            case ']': AddToken(ETokenType::RIGHT_BRACKET); break;
            case ',': AddToken(ETokenType::COMMA); break;
            case '.': AddToken(ETokenType::DOT); break;
            case ':': AddToken(ETokenType::COLON); break;
            case '-': AddToken(ETokenType::MINUS); break;
            case '+': AddToken(ETokenType::PLUS); break;
            case ';': AddToken(ETokenType::SEMICOLON); break;
            case '*': AddToken(ETokenType::STAR); break;
            
            case '!': AddToken(Match('=') ? ETokenType::BANG_EQUAL : ETokenType::BANG); break;
            case '=': AddToken(Match('=') ? ETokenType::EQUAL_EQUAL : ETokenType::EQUAL); break;
            case '<': AddToken(Match('=') ? ETokenType::LESS_EQUAL : ETokenType::LESS); break;
            case '>': AddToken(Match('=') ? ETokenType::GREATER_EQUAL : ETokenType::GREATER); break;
            
            case '/':
                if (Match('/'))
                {
                    // Comment - skip to end of line
                    while (Peek() != '\n' && !IsAtEnd()) Advance();
                }
                else
                {
                    AddToken(ETokenType::SLASH);
                }
                break;
            
            case ' ':
            case '\r':
            case '\t':
                break;
            
            case '\n':
                Line++;
                Column = 1;
                break;
            
            case '"':
                ScanString();
                break;
            
            default:
                if (std::isdigit(c))
                {
                    ScanNumber();
                }
                else if (std::isalpha(c) || c == '_')
                {
                    ScanIdentifier();
                }
                else
                {
                    std::cerr << "[ERROR] Unexpected character '" << c << "' at line " << Line << std::endl;
                }
                break;
        }
    }
    
    void ScanString()
    {
        while (Peek() != '"' && !IsAtEnd())
        {
            if (Peek() == '\n')
            {
                Line++;
                Column = 1;
            }
            Advance();
        }
        
        if (IsAtEnd())
        {
            std::cerr << "[ERROR] Unterminated string at line " << Line << std::endl;
            return;
        }
        
        Advance(); // Closing "
        
        FString value = Source.substr(Start + 1, Current - Start - 2);
        AddToken(ETokenType::STRING, value);
    }
    
    void ScanNumber()
    {
        while (std::isdigit(Peek())) Advance();
        
        if (Peek() == '.' && std::isdigit(PeekNext()))
        {
            Advance(); // Consume .
            while (std::isdigit(Peek())) Advance();
        }
        
        FString value = Source.substr(Start, Current - Start);
        FScriptToken token(ETokenType::NUMBER, value, Line, Column - static_cast<int32>(value.length()));
        token.NumberValue = std::stod(value);
        Tokens.push_back(token);
    }
    
    void ScanIdentifier()
    {
        while (std::isalnum(Peek()) || Peek() == '_') Advance();
        
        FString text = Source.substr(Start, Current - Start);
        ETokenType type = GetKeywordType(text);
        AddToken(type, text);
    }
    
    ETokenType GetKeywordType(const FString& text) const
    {
        if (text == "and") return ETokenType::AND;
        if (text == "break") return ETokenType::BREAK;
        if (text == "case") return ETokenType::CASE;
        if (text == "class") return ETokenType::CLASS;
        if (text == "const") return ETokenType::CONST;
        if (text == "continue") return ETokenType::CONTINUE;
        if (text == "default") return ETokenType::DEFAULT;
        if (text == "do") return ETokenType::DO;
        if (text == "else") return ETokenType::ELSE;
        if (text == "enum") return ETokenType::ENUM;
        if (text == "false") return ETokenType::KW_FALSE;
        if (text == "float") return ETokenType::FLOAT;
        if (text == "for") return ETokenType::FOR;
        if (text == "function") return ETokenType::FUNCTION;
        if (text == "if") return ETokenType::IF;
        if (text == "import") return ETokenType::IMPORT;
        if (text == "int") return ETokenType::INT;
        if (text == "nil") return ETokenType::NIL;
        if (text == "or") return ETokenType::OR;
        if (text == "print") return ETokenType::PRINT;
        if (text == "return") return ETokenType::RETURN;
        if (text == "string") return ETokenType::STRING_TYPE;
        if (text == "struct") return ETokenType::STRUCT;
        if (text == "super") return ETokenType::SUPER;
        if (text == "switch") return ETokenType::SWITCH;
        if (text == "this") return ETokenType::THIS;
        if (text == "true") return ETokenType::KW_TRUE;
        if (text == "typedef") return ETokenType::TYPEDEF;
        if (text == "var") return ETokenType::VAR;
        if (text == "void") return ETokenType::VOID;
        if (text == "while") return ETokenType::WHILE;
        if (text == "public") return ETokenType::PUBLIC;
        if (text == "private") return ETokenType::PRIVATE;
        
        return ETokenType::IDENTIFIER;
    }
    
public:
    TArray<FScriptToken> Tokenize(const FString& source)
    {
        Source = source;
        Start = 0;
        Current = 0;
        Line = 1;
        Column = 1;
        Tokens.clear();
        
        while (!IsAtEnd())
        {
            Start = Current;
            ScanToken();
        }
        
        Tokens.push_back(FScriptToken(ETokenType::END_OF_FILE, "", Line, Column));
        return Tokens;
    }
};

// Stub native function for Log (FScriptValue is defined in ScriptBytecode.h)
void StubLog(FScriptVM* VM)
{
    std::cout << "[SCRIPT] ";
    for (const auto& arg : args)
    {
        std::cout << arg.ToString();
    }
    std::cout << std::endl;
}

void PrintUsage()
{
    std::cout << "Custom C Script Compiler & VM - Standalone Console" << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "  ScriptCompiler compile <input.sbs> [-o <output.sbc>]" << std::endl;
    std::cout << "  ScriptCompiler run <script.sbs>" << std::endl;
    std::cout << "  ScriptCompiler exec <bytecode.sbc>" << std::endl;
    std::cout << "  ScriptCompiler test" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  ScriptCompiler compile Test.sbs -o Test.sbc" << std::endl;
    std::cout << "  ScriptCompiler run Test.sbs" << std::endl;
    std::cout << "  ScriptCompiler test" << std::endl;
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        PrintUsage();
        return 1;
    }
    
    FString command = argv[1];
    
    if (command == "compile")
    {
        if (argc < 3)
        {
            std::cerr << "Error: No input file specified" << std::endl;
            return 1;
        }
        
        FString inputPath = argv[2];
        FString outputPath = inputPath;
        
        // Replace .sbs with .sbc
        size_t dotPos = outputPath.rfind('.');
        if (dotPos != FString::npos)
        {
            outputPath = outputPath.substr(0, dotPos) + ".sbc";
        }
        else
        {
            outputPath += ".sbc";
        }
        
        // Check for -o flag
        for (int i = 3; i < argc - 1; i++)
        {
            if (std::string(argv[i]) == "-o")
            {
                outputPath = argv[i + 1];
                break;
            }
        }
        
        std::cout << "Compiling: " << inputPath << " -> " << outputPath << std::endl;
        
        // Load source
        FString source;
        if (!FPlatformFile_Utils::LoadFileToString(source, inputPath))
        {
            std::cerr << "Error: Could not read file: " << inputPath << std::endl;
            return 1;
        }
        
        // Tokenize
        std::cout << "Tokenizing..." << std::endl;
        FScriptLexer lexer;
        TArray<FScriptToken> tokens = lexer.Tokenize(source);
        std::cout << "  Tokens: " << tokens.size() << std::endl;
        
        // Parse
        std::cout << "Parsing..." << std::endl;
        FScriptParser parser(tokens);
        TSharedPtr<FScriptProgram> program = parser.Parse();
        
        if (!program || !program->IsValid())
        {
            std::cerr << "Parse failed!" << std::endl;
            auto errors = parser.GetErrors();
            for (const auto& error : errors)
            {
                std::cerr << "  " << error << std::endl;
            }
            return 1;
        }
        
        std::cout << "  Functions: " << program->Functions.size() << std::endl;
        std::cout << "  Statements: " << program->Statements.size() << std::endl;
        
        // Compile
        std::cout << "Compiling to bytecode..." << std::endl;
        FScriptCompiler compiler;
        TSharedPtr<FBytecodeChunk> bytecode = compiler.Compile(program);
        
        if (!bytecode)
        {
            std::cerr << "Compilation failed!" << std::endl;
            auto errors = compiler.GetErrors();
            for (const auto& error : errors)
            {
                std::cerr << "  " << error << std::endl;
            }
            return 1;
        }
        
        std::cout << "  Bytecode size: " << bytecode->Code.size() << " bytes" << std::endl;
        std::cout << "  Constants: " << bytecode->Constants.size() << std::endl;
        std::cout << "  Functions: " << bytecode->Functions.size() << std::endl;
        
        // Save bytecode (simplified - just save code for now)
        if (!FPlatformFile_Utils::SaveArrayToFile(bytecode->Code, outputPath))
        {
            std::cerr << "Error: Could not write output file: " << outputPath << std::endl;
            return 1;
        }
        
        std::cout << "Success! Wrote " << bytecode->Code.size() << " bytes to " << outputPath << std::endl;
        return 0;
    }
    else if (command == "run")
    {
        if (argc < 3)
        {
            std::cerr << "Error: No input file specified" << std::endl;
            return 1;
        }
        
        FString inputPath = argv[2];
        std::cout << "Running: " << inputPath << std::endl;
        
        // Load source
        FString source;
        if (!FPlatformFile_Utils::LoadFileToString(source, inputPath))
        {
            std::cerr << "Error: Could not read file: " << inputPath << std::endl;
            return 1;
        }
        
        // Tokenize
        FScriptLexer lexer;
        TArray<FScriptToken> tokens = lexer.Tokenize(source);
        
        // Parse
        FScriptParser parser(tokens);
        TSharedPtr<FScriptProgram> program = parser.Parse();
        
        if (!program || !program->IsValid())
        {
            std::cerr << "Parse failed!" << std::endl;
            auto errors = parser.GetErrors();
            for (const auto& error : errors)
            {
                std::cerr << "  " << error << std::endl;
            }
            return 1;
        }
        
        // Compile
        FScriptCompiler compiler;
        TSharedPtr<FBytecodeChunk> bytecode = compiler.Compile(program);
        
        if (!bytecode)
        {
            std::cerr << "Compilation failed!" << std::endl;
            auto errors = compiler.GetErrors();
            for (const auto& error : errors)
            {
                std::cerr << "  " << error << std::endl;
            }
            return 1;
        }
        
        std::cout << "Compiled successfully. Executing..." << std::endl;
        std::cout << "======================================" << std::endl;
        
        // Execute
        auto startTime = std::chrono::high_resolution_clock::now();
        
        FScriptVM vm;
        bool success = vm.Execute(bytecode);
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        
        std::cout << "======================================" << std::endl;
        
        if (!success)
        {
            std::cerr << "Execution failed!" << std::endl;
            auto errors = vm.GetErrors();
            for (const auto& error : errors)
            {
                std::cerr << "  " << error << std::endl;
            }
            return 1;
        }
        
        // Try to call Main()
        if (vm.CallMainIfExists())
        {
            std::cout << "Main() executed successfully" << std::endl;
        }
        
        std::cout << "Execution time: " << duration.count() << " microseconds" << std::endl;
        return 0;
    }
    else if (command == "test")
    {
        std::cout << "Running integrated tests..." << std::endl;
        std::cout << "Test functionality not yet implemented" << std::endl;
        return 0;
    }
    else
    {
        std::cerr << "Unknown command: " << command << std::endl;
        PrintUsage();
        return 1;
    }
}

