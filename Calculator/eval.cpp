/*
 * \brief ���ʽ������
 * \author Perdixky
 * \note ������Ĵ����Ϊ���֣��ֱ��ǷǷ��ַ����󣬱��ʽ������������
 * \note ������pos��������ÿ��������ĩβ����֤�ڷ���ʱposָ����һ��token
 * \note �����ж�Խ��Ĵ�������parseAtom
 * \note ������ʹ����C++23��ģ�黯���ԣ���Ҫʹ��C++23������
 */
export module calculator;
import std;
import <conio.h>;


class syntax_error final : public std::runtime_error //���ʽ�����࣬�����׳����󣬰���������Ϣ�ʹ���λ�ã�λ����tokens�е�λ��Ϊ׼
{
public:
	explicit syntax_error(const std::string_view message, const std::pair<size_t, size_t> position) : std::runtime_error(message.data()), position_(position) { }
	[[nodiscard]] auto position() const -> std::pair<size_t, size_t>{
		return position_;
	}
private:
	std::pair<size_t, size_t> position_;
};

class operate_error final : public std::runtime_error //��������࣬�����׳����󣬽�����������Ϣ
{
public:
	explicit operate_error(const std::string_view message) : std::runtime_error(message.data()) { }
};

enum class TokenType  //����ö��
{
	Number, Function, Operator, Parenthesis
};
class Token
{
public:
	Token(const TokenType type, const std::variant<double, std::unordered_map<std::string, std::function<double(double)>>::const_iterator, char>& value, const std::pair<size_t, size_t> position) : type(type), value(value), position(position) { }  //���캯��
	TokenType type;
	std::variant<double, std::unordered_map<std::string, std::function<double(double)>>::const_iterator, char> value;
	std::pair<size_t, size_t> position;
};

/*
 * \brief Token���͵����������Ŀǰû��
 */
template<>
struct std::formatter<std::variant<double, std::unordered_map<std::string, std::function<double(double)>>::const_iterator, char>> {
	// parse ����Ϊ�գ���Ϊ���ǲ������κθ�ʽ��ѡ��
	static constexpr auto parse(std::format_parse_context& ctx) -> decltype(ctx.begin()) {
		return ctx.begin();
	}

	// format ������Ҫ����ͬ���͵� variant
	template<typename FormatContext>
	auto format(const std::variant<double, std::unordered_map<std::string, std::function<double(double)>>::const_iterator, char>& var, FormatContext& ctx) const -> decltype(ctx.out()) {
		return std::visit([&ctx]<typename T0>(const T0& val) -> decltype(ctx.out()) {
			using T = std::decay_t<T0>;
			if constexpr (std::is_same_v<T, double>) {
				return std::format_to(ctx.out(), "Number: {}", val);
			}
			else if constexpr (std::is_same_v<T, std::unordered_map<std::string, std::function<double(double)>>::const_iterator>) {
				// ���躯��������ָ��ĵ�һ��Ԫ�صļ��Ǻ�������
				return std::format_to(ctx.out(), "Function: {}", val->first);
			}
			else if constexpr (std::is_same_v<T, char>) {
				return std::format_to(ctx.out(), "Operator or Parenthesis: {}", val);
			}
		}, 
		var);
	}
};

/*
 * \brief ����ӳ������ڴ洢�������ͺ���ָ�룬ʹ��unordered_map����Ϊunordered_map�Ĳ����ٶȸ���
 */
export const std::unordered_map<std::string, std::function<double(double)>> Functions
{
	{"sin",     [](const double arg) { return std::sin(arg); }  },
	{"cos",     [](const double arg) { return std::cos(arg); }  },
	{"tan",     [](const double arg)
	{
		if (std::fmod(arg, std::numbers::pi) == std::numbers::pi / 2)
			throw operate_error("tan�����Ĳ��������Ǧ�/2+k�У�");
		return std::tan(arg);
	}  },
	{"arcsin",  [](const double arg)
	{
		if (arg < -1 || arg > 1)
			throw operate_error("arcsin�����Ĳ���������-1��1֮�䣡");
		return std::asin(arg);
	} },
	{"arccos",  [](const double arg)
	{
		if (arg < -1 || arg > 1)
			throw operate_error("arccos�����Ĳ���������-1��1֮�䣡");
		return std::acos(arg);
	} },
	{"arctan",  [](const double arg) { return std::atan(arg); } },
	{"sinh",    [](const double arg) { return std::sinh(arg); } },
	{"cosh",    [](const double arg) { return std::cosh(arg); } },
	{"tanh",    [](const double arg) { return std::tanh(arg); } },
	{"arcsinh", [](const double arg) { return std::asinh(arg); }},
	{"arccosh", [](const double arg) { return std::acosh(arg); }},
	{"arctanh", [](const double arg) { return std::atanh(arg); }},
	{"ln",      [](const double arg)
	{
		if (arg < 0)
			throw operate_error("���������Ĳ�������С��0��");
		return std::log(arg);
	}  },
	{"lg",      [](const double arg)
	{
		if (arg < 0)
			throw operate_error("���������Ĳ�������С��0��");
		return std::log10(arg);
	}},
	{"exp",     [](const double arg) { return std::exp(arg); }  },
	{"sqrt",    [](const double arg)
	{
		if (arg < 0)
			throw operate_error("�������ܿ�ƽ����");
		return std::sqrt(arg);
	} },
	{"cbrt",    [](const double arg) { return std::cbrt(arg); } },  //������
	{"abs",     [](const double arg) { return std::abs(arg); }  },
};

/*
 * \brief �ڵ��࣬��Ϊԭ�ӽڵ㣬һԪ�ڵ�Ͷ�Ԫ�ڵ㣬������ͬ�Ļ�����Ϊ��ʵ�ֶ�̬
 */
class Node 
{
public:
	Node() = default;  //���캯��
	virtual ~Node() = default;
	[[nodiscard]] virtual auto get_value() const -> double = 0;  //��ȡֵ�����麯��
	[[nodiscard]] virtual auto is_operator() const -> bool{
		return false;
	};
};

class AtomicNode final : public Node //ԭ�ӽڵ���
{
public:
	explicit AtomicNode(const double value) : value_(value) { }  //���캯��
	[[nodiscard]] auto get_value() const -> double override;  //��ȡֵ
	
private:
	double value_;  //ֵ
};

class UnaryNode final : public Node  //һԪ�ڵ��࣬�洢������������
{
	public:
	UnaryNode(std::unique_ptr<Node> below, const std::function<double(double)>& function) : function_(function), below_(std::move(below)) { }
	[[nodiscard]] auto get_value() const -> double override;  //��ȡֵ

private:
	std::function<double(double)> function_;  //����ָ��
	std::unique_ptr<Node> below_;  //�ӽڵ�
};

class BinaryNode final : public Node  //��Ԫ�ڵ���
{
public:
	BinaryNode(const char op, std::unique_ptr<Node> left, std::unique_ptr<Node> right) : op_(op), left_(std::move(left)), right_(std::move(right)) {}  //���캯��
	[[nodiscard]] auto get_value() const -> double override;  //��ȡֵ

private:
	char op_;  //������
	std::unique_ptr<Node> left_;
	std::unique_ptr<Node> right_;  //�����ӽڵ�
};

auto AtomicNode::get_value() const -> double  //��ȡֵ
{
	return value_;
}
auto UnaryNode::get_value() const -> double  //��ȡֵ
{
	return function_(below_->get_value());
}
auto BinaryNode::get_value() const -> double
{
	const double left = left_->get_value();
	const double right = right_->get_value();
	switch (op_)
	{
	case '+':
		return left + right;
	case '-':
		return left - right;
	case '*':
		return left * right;
	case '/':
		if (right == 0)
		{
			throw operate_error("��������Ϊ0��");
		}
		return left / right;
	case '^':
		if (left == 0 && right < 0)
		{
			throw operate_error("0�ĸ����������壡");
		}
		if(left < 0 && std::floor(right) != right)
		{
			throw operate_error("�����ķ��������������壡");
		}
		return std::pow(left, right);
	default:
		return 0;  //���return���ᱻִ�У�ֻ��Ϊ����������
	}
}

auto parseExpression(size_t& pos, const std::vector<Token>& tokens) -> std::unique_ptr<Node>;  //��������ΪparseExpression��parseAtom������ã�������Ҫ��ǰ����
/*
 * \brief ����ԭ�ӣ������ֺ������ڱ��ʽ
 */
auto parseAtom(size_t& pos, const std::vector<Token>& tokens) -> std::unique_ptr<Node>
{
	try
	{
		if (tokens.at(pos).type == TokenType::Parenthesis)
		{
			if (std::get<char>(tokens.at(pos).value) == '(')  //ʹ��.at()������[]����Ϊ.at()����Խ�粢�׳��쳣
			{
				pos++;
				std::unique_ptr<Node> atom = parseExpression(pos, tokens);
				if (std::get<char>(tokens.at(pos).value) != ')')
				{
					throw syntax_error("ȱ�������ţ�", tokens[pos].position);
				}
				pos++;
				return atom;
			}
		}
		return std::make_unique<AtomicNode>(std::get<double>(tokens.at(pos++).value));  //��ʹ��pos����������֤����ʱposָ����һ��token
	}
	catch (std::out_of_range&){
		pos--;  //ȷ������Խ��
		throw syntax_error("ȱ�����ֻ����ţ�", tokens[pos].position);
	}
	catch (const std::bad_variant_access&)
	{
		throw syntax_error("�˴�ӦΪ���֣�", tokens[pos].position);
	}
}

/*
 * \brief ����һԪ�ڵ㣬�������źͺ���
 */
auto parseFunction(size_t& pos, const std::vector<Token>& tokens) -> std::unique_ptr<Node> {
	while (true) {
		if (pos >= tokens.size()){
			break;  //ȷ������Խ��
		}
		if (tokens[pos].type == TokenType::Operator) {
			if (const char op = std::get<char>(tokens[pos].value); op == '-' || op == '+') {
				pos++;
				std::unique_ptr<Node> below = parseFunction(pos, tokens);  //����ʹ�õݹ飬��Ϊ�����źͺ���������������
				if (op == '-') 
					return std::make_unique<UnaryNode>(std::move(below), [](const double arg) { return -arg; });
				return std::make_unique<UnaryNode>(std::move(below), [](const double arg) { return arg; });
			}
		}
		else if (tokens[pos].type == TokenType::Function) {
			const auto iterator = std::get<std::unordered_map<std::string, std::function<double(double)>>::const_iterator>(tokens[pos].value);
			pos++;
			return std::make_unique<UnaryNode>(std::move(parseFunction(pos, tokens)), iterator->second);
		}
		else
			break; // �˳�ѭ��������ǲ������ͷǺ��������
	}
	return parseAtom(pos, tokens);
}

/*
 * \brief �������ӣ�������ָ������
 */
auto parseFactor(size_t& pos, const std::vector<Token>& tokens) -> std::unique_ptr<Node>
{
	std::unique_ptr<Node> left = std::move(parseFunction(pos, tokens));
	if(pos >= tokens.size()){
		return left;  //ȷ������Խ��
	}
	try{
		while (true) {
			const char op = std::get<char>(tokens[pos].value);
			if (op != '^') {
				break; // ������ǼӺŻ���ţ�������ѭ��
			}
			pos++; // �ƶ�����һ������
			if (pos >= tokens.size())
				throw syntax_error("ȱ��ָ����", tokens[--pos].position); //�������ʹ��--pos�����Խ��
			if (tokens[pos].type == TokenType::Operator)
				throw syntax_error("ָ�����������Ӧ�������ֻ����ţ�", tokens[pos].position);
			std::unique_ptr<Node> right = std::move(parseFunction(pos, tokens));
			left = std::make_unique<BinaryNode>(op, std::move(left), std::move(right)); // ������Ԫ�ڵ�
			if (pos >= tokens.size()) {
				break; // ȷ������Խ��
			}
		}
	}catch (const std::bad_variant_access&)
	{
		throw syntax_error("���ʽ�ṹ���󣡴˴�ӦΪ�����", tokens[pos].position);
	}  //try-catchֻ��Ҫ�ڴ˴�ʹ��һ�Σ���ΪֻҪ��ȷ����char���ͣ���һ���������
	return left;
}

/*
 * \brief ��������˳�
 */
auto parseTerm(size_t& pos, const std::vector<Token>& tokens) -> std::unique_ptr<Node>
{
	std::unique_ptr<Node> left = std::move(parseFactor(pos, tokens));
	if (pos >= tokens.size()) {
		return left;  //ȷ������Խ��
	}
	while (true) {
		const char op = std::get<char>(tokens[pos].value);
		if (op != '*' && op != '/') {
			break; // ������ǼӺŻ���ţ�������ѭ��
		}
		pos++; // �ƶ�����һ������
		if (pos >= tokens.size())
			throw syntax_error("ȱ�ٳ˳��ź�������ӣ�", tokens[--pos].position);
		if (tokens[pos].type == TokenType::Operator)
			throw syntax_error("�˳��ź���Ӧ�������ֻ����ţ�", tokens[pos].position);
		std::unique_ptr<Node> right = std::move(parseFactor(pos, tokens));
		left = std::make_unique<BinaryNode>(op, std::move(left), std::move(right)); // ������Ԫ�ڵ�
		if (pos >= tokens.size()) {
			break; // ȷ������Խ��
		}
	}
	return left;
}

/*
 * \brief �������ʽ�����Ӽ�
 */
auto parseExpression(size_t& pos, const std::vector<Token>& tokens) -> std::unique_ptr<Node>
{
	std::unique_ptr<Node> left = std::move(parseTerm(pos, tokens));
	if (pos >= tokens.size()) {
		return left;  //ȷ������Խ��
	}
	while (true) {
		const char op = std::get<char>(tokens[pos].value);
		if (op != '+' && op != '-') {
			break;  //������ǼӺŻ���ţ�������ѭ��
		}
		pos++;  //�ƶ�����һ������
		if (pos >= tokens.size())
			throw syntax_error("ȱ�ټӼ��ź�����", tokens[--pos].position);
		if (tokens[pos].type == TokenType::Operator)
			throw syntax_error("�Ӽ��ź���Ӧ�������ֻ����ţ�", tokens[pos].position);
		std::unique_ptr<Node> right = std::move(parseTerm(pos, tokens));
		left = std::make_unique<BinaryNode>(op, std::move(left), std::move(right)) ; // ������Ԫ�ڵ�
		if (pos >= tokens.size()) {
			break;  //ȷ������Խ��
		}
	}
	return left;
}

/*
 * \brief �ӱ��ʽ�л�ȡtokens
 * \param expression ���ʽ
 */
auto get_tokens(const std::string_view expression) -> std::vector<Token>
{
	std::vector<Token> tokens;
	std::string number;
	std::string function;

	auto function_flush = [&function, &tokens](const size_t i)  //�������������ݳ�ϴ��tokens��
		{
			if (!function.empty())
			{
				if (auto iterator = Functions.find(function); iterator != Functions.end())
					tokens.emplace_back(TokenType::Function, iterator, std::pair(i - function.size(), i - 1));
				else
					throw syntax_error(std::format("�޷�����ĺ�����{}", function), std::pair(i - function.size(), i - 1));
				function.clear();
			}
		};
	auto number_flush = [&number, &tokens](const size_t i)
		{
			if (!number.empty())
			{
				try
				{
					tokens.emplace_back(TokenType::Number, std::stod(number), std::pair(i - number.size(), i - 1));
					number.clear();
				}
				catch (const std::invalid_argument&) {
					throw syntax_error("�޷���������֣�", std::pair(i - number.size(), i - 1));
				}
			}
		};
	for (size_t i{ 0 }; i < expression.size(); i++)
	{
		if (const char c = expression[i]; std::isdigit(static_cast<unsigned char>(c)) || c == '.') {
			function_flush(i);
			number += c;
		}
		else if (std::isalpha(static_cast<unsigned char>(c))) {
			number_flush(i);
			function += c;
		}
		else if (std::isspace(static_cast<unsigned char>(c)))
		{
			number_flush(i);
			function_flush(i);
		}
		else
		{
			number_flush(i);
			function_flush(i);
			switch (c) {
			case '+':
			case '-':
			case '*':
			case '/':
			case '^':
				tokens.emplace_back(TokenType::Operator, c, std::pair(i, i));
				break;
			case '(':
			case ')':
				tokens.emplace_back(TokenType::Parenthesis, c, std::pair(i, i));
				break;
			default:
				throw syntax_error("�Ƿ��ַ���", std::pair(i, i));
			}
		}
	}
	// ������ʽĩβ�����ֻ�������ʹ�����������ֲ��ᱻ����
	number_flush(expression.size());
	function_flush(expression.size());
	return tokens;  // ����NRVO������ʹ��std::move
}

export auto eval(const std::string_view expression) -> double
{
	try {
		size_t pos{ 0 };  // �ǳ������õĳ�ʼֵ��������ֵ
		return parseExpression(pos, std::forward<std::vector<Token>>(get_tokens(expression)))->get_value();
	}
	catch (syntax_error& e)
	{
		for(size_t i = 0; i < e.position().first; i++)
		{
			std::print(" ");
		}
		for(size_t i = e.position().first; i <= e.position().second; i++)
		{
			std::print("^");
		}
		std::print("\n");
		std::println("{}", e.what());
		return std::nan("1");
	}
	catch (operate_error& e)
	{
		std::println("{}", e.what());
		return std::nan("1");
	}
	catch(...)
	{
		std::println("δ֪���󣡼����Ƴ����򡭡�\n�븴�����ı��ʽ����������");
		std::println("�밴�������������");
		_getch();
		return std::nan("1");
	}
}