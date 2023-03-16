#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>

#include <stdio.h>
#include <string.h>
#include <stack>
#include <queue>
#include <string>
#include <sstream>

static WNDPROC OldEditProc;

class Token
{
public:
	Token() = delete;
	using TokenType = enum class Type
	{
		UNKNOWN,
		OPERATOR,
		NUMBER,
	};
	Token(TokenType type, std::wstring const& value) : m_type(type), m_value(value)
	{

	}
	TokenType Type() { return m_type; }
	std::wstring Value() const { return m_value; }

private:
	TokenType m_type;
	std::wstring m_value;
};
class Lexer
{
	std::wstring m_data;
	size_t m_at;

public:
	Lexer(Lexer&) = delete;
	explicit Lexer(std::wstring const& data) : m_data(data), m_at(0)
	{

	}

	Token Next()
	{

		if (iswdigit(m_data[m_at]))
		{
			std::wstring Number = ReadNumber();
			return Token(Token::TokenType::NUMBER, Number);
		}
		else if (IsOperator(m_data[m_at]))
		{
			std::wstring Operator = ReadOperator();
			return Token(Token::TokenType::OPERATOR, Operator);
		}
		return Token{ Token::TokenType::UNKNOWN, L"Unknown" };
	}

private:
	bool IsOperator(wchar_t Char)
	{
		return Char == L'+'
			|| Char == L'-'
			|| Char == L'*'
			|| Char == L'/';
	}
	std::wstring ReadNumber()
	{
		std::wstringstream Result;
		wchar_t Char = m_data[m_at];
		while (iswdigit(Char))
		{
			Result << Char;
			Char = m_data[++m_at];
		}
		return Result.str();
	}

	std::wstring ReadOperator()
	{
		std::wstringstream Result;
		wchar_t Char = m_data[m_at];
		if (IsOperator(Char))
		{
			Result << Char;
			++m_at;
		}
		return Result.str();
	}

};

LRESULT CALLBACK EditSubProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
		// https://learn.microsoft.com/en-us/windows/win32/controls/subclass-a-combo-box

	case WM_KILLFOCUS:
	{
		int Length = GetWindowTextLengthW(hwnd);
		if (Length > 0)
		{
			HANDLE hBuffer = HeapCreate(0, 0, (size_t)(64 * 1024) * sizeof(wchar_t));
			if (hBuffer)
			{
				LPVOID Buffer = HeapAlloc(hBuffer, HEAP_ZERO_MEMORY, sizeof(wchar_t) * (size_t)(Length + 1));
				if (Buffer)
				{
					GetWindowTextW(hwnd, (wchar_t*)Buffer, Length + 1);
					std::stack<Token> Numbers;
					std::stack<Token> Operators;

					Lexer lexer((wchar_t*)Buffer);
					Token current = lexer.Next();
					while (current.Type() != Token::TokenType::UNKNOWN)
					{
						switch (current.Type())
						{
						case Token::TokenType::NUMBER:
							Numbers.push(current);
							break;
						case Token::TokenType::OPERATOR:
							Operators.push(current);
							break;
						}
						current = lexer.Next();
					}

					if (!Operators.empty())
					{
						auto& Operator = Operators.top();
						if (Operator.Value() == L"+")
						{
							auto A = _wtoi(Numbers.top().Value().c_str()); Numbers.pop();
							auto B = _wtoi(Numbers.top().Value().c_str()); Numbers.pop();
							SetWindowTextW(hwnd, std::to_wstring(A + B).c_str());
						}
						else if (Operator.Value() == L"-")
						{
							auto A = _wtoi(Numbers.top().Value().c_str()); Numbers.pop();
							auto B = _wtoi(Numbers.top().Value().c_str()); Numbers.pop();
							SetWindowTextW(hwnd, std::to_wstring(A - B).c_str());
						}
						else if (Operator.Value() == L"/")
						{
							auto B = _wtoi(Numbers.top().Value().c_str()); Numbers.pop();
							auto A = _wtoi(Numbers.top().Value().c_str()); Numbers.pop();
							SetWindowTextW(hwnd, std::to_wstring(A / B).c_str());

						}
						else if (Operator.Value() == L"*")
						{
							auto A = _wtoi(Numbers.top().Value().c_str()); Numbers.pop();
							auto B = _wtoi(Numbers.top().Value().c_str()); Numbers.pop();
							SetWindowTextW(hwnd, std::to_wstring(A * B).c_str());
						}
					}
					HeapFree(hBuffer, 0, Buffer);
				}
			}


		}
	}
	break;
	default:
		break;
	}

	return CallWindowProcW(OldEditProc, hwnd, msg, wparam, lparam);
}
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	static HWND edit1, edit2;

	switch (msg)
	{
	case WM_CREATE:
	{
		RECT ClientRect{};
		GetClientRect(hwnd, &ClientRect);

		int Width = ClientRect.right - ClientRect.left;
		int Height = ClientRect.bottom - ClientRect.top;

		edit1 = CreateWindow(L"edit", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 10, 10, 100, 25, hwnd, nullptr, GetModuleHandle(0), nullptr);
		edit2 = CreateWindow(L"edit", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 10, 35, 100, 25, hwnd, nullptr, GetModuleHandle(0), nullptr);

		OldEditProc = (WNDPROC)SetWindowLongPtr(edit2, GWLP_WNDPROC, (LONG_PTR)EditSubProc);
	}
	return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		break;
	}
	return DefWindowProcW(hwnd, msg, wparam, lparam);
}


int main()
{
	WNDCLASSEX wc{};
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.hInstance = GetModuleHandle(0);
	wc.lpszClassName = L"CSubclass01";
	wc.lpfnWndProc = &WndProc;

	RegisterClassEx(&wc);

	HWND hwnd = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW, wc.lpszClassName, L"Subclass Example", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, GetModuleHandle(0), nullptr);
	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	return msg.wParam;
}