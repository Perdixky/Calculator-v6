import std;
import calculator;
import <replxx.hxx>;
import <Windows.h>;

auto main() -> int
{
	replxx::Replxx rx;
	rx.install_window_change_handler();
	rx.set_completion_callback([](const std::string_view expression, int& token_lenth) -> replxx::Replxx::completions_t
	{
		replxx::Replxx::completions_t completions; //候选名单
		const std::string_view present_token = expression.substr(expression.find_last_of(' ') + 1); //当前输入的单词
		if (present_token.empty()) //如果当前输入的单词为空
		{
			token_lenth = 0;
			return completions;
		}
		for (const std::string& function : Functions | std::views::keys)
		{
			if(function.starts_with(present_token))
			{
				completions.emplace_back(function.c_str());
			}
		}
		token_lenth = static_cast<int>(present_token.length());
		return completions;
	});
	rx.set_hint_callback([](const std::string_view& expression, int& token_lenth, replxx::Replxx::Color& color) -> replxx::Replxx::hints_t {
		replxx::Replxx::hints_t hints; // 提示名单
		const std::string_view present_token = expression.substr(expression.find_last_of(' ') + 1); //当前输入的单词
		if (present_token.empty()) //如果当前输入的单词为空
		{
			token_lenth = 0;
			return hints;
		}
		for (const std::string& function : Functions | std::views::keys) {
			if (function.starts_with(present_token))
			{
				hints.emplace_back(function.c_str());
			}
		}
		color = replxx::Replxx::Color::GRAY;
		token_lenth = static_cast<int>(present_token.length());
		return hints;
	});
	do {
		char const* cinput = rx.input("");
		if (cinput == nullptr) { // 用户输入Ctrl+C或Ctrl+D
			break;
		}
		std::print("{}\n", eval(cinput));
	} while (true);

	return 0;
}