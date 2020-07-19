/*
	SerpApi:

	Optimization is about making assumptions.

	Assumptions come at the cost of compatibility.



	This is the balancing act of high performance code. This tension is the tight rope we walk on top of in this space.



	There are many directions one can go with an open ended code challenge like this one.

	Given that my directions were to complete it in C++, and that the position is one of library maintenence / upgrade I've opted to focus on the following:

	1. Reusable Code Library.
	2. 100% Custom Implementations.
	3. Optimization Focused ( Assumtpion Heavy, Compatability Light ) without being too brittle.

	Cheers!
*/

#pragma once

#include <string_view>
#include <string>



namespace ege
{
	/*
		switch_t... Nice to have, see https://github.com/justdoityourself/d8u
	*/

	namespace d8u
	{
		uint64_t constexpr __mix(char m, uint64_t s)
		{
			return ((s << 7) + ~(s >> 3)) + ~m;
		}

		uint64_t constexpr _switch_t(const char* m)
		{
			return (*m) ? __mix(*m, _switch_t(m + 1)) : 0;
		}

		uint64_t constexpr _switch_t(const char* m, size_t l)
		{
			return (l) ? __mix(*m, _switch_t(m + 1, l - 1)) : 0;
		}

		uint64_t constexpr switch_t(std::string_view t)
		{
			return _switch_t((const char*)t.data(), t.size());
		}

		uint64_t constexpr switch_t(const char* s)
		{
			std::string_view t(s);
			return _switch_t((const char*)t.data(), t.size());
		}
	}

	namespace XML
	{
		/*
			Provides a basic engine to parse through the outline of an XML Document.

			Callback is passed an OutlineContext that describes the current element.

			This function assumes minified HTML.
		*/

		struct OutlineContext
		{
			size_t depth = 0;
			size_t parameter_start = -1;
			
			std::string_view type;
			std::string_view parameters;

			void Reset()
			{
				parameter_start = -1;

				type = std::string_view();
				parameters = std::string_view();
			}
		};

		template < typename ELEMENT_CALLBACK > void Outline(std::string_view data, ELEMENT_CALLBACK&& cb)
		{
			/*
				The main loop reserve a peek ahead and therefore stops before the end of the buffer.
			*/

			constexpr std::string_view _comment("!--");

			OutlineContext context;

			constexpr size_t peek_length = 1;
			size_t i = 0;
			bool active = true, string = false, string2 = false;

			auto submit = [&]()
			{
				/*
					Ignore Comments
				*/

				if (_comment == context.type)
				{
					context.depth--;
					context.Reset();
					return;
				}

				/*
					Self Closing elements
				*/

				if ('/' == data[i - 1])
					context.depth--;
				else
					/*
						Void elements
					*/

					switch (d8u::switch_t(context.type))
					{
					default: break;
					case d8u::switch_t("img"):
					case d8u::switch_t("input"):
					case d8u::switch_t("br"):
					case d8u::switch_t("meta"):
					case d8u::switch_t("param"):
					case d8u::switch_t("link"):
					case d8u::switch_t("source"):
					case d8u::switch_t("track"):

						context.depth--;
						break;
					}



				if (context.parameter_start != -1)
				{
					context.parameters = std::string_view(data.data() + context.parameter_start, i - context.parameter_start);

					active = cb(context);

					context.Reset();
				}
			};

			for (; active && i < data.size() - peek_length; i++)
			{
				if (string && data[i] != '"')
					continue;

				if (string2 && data[i] != '\'')
					continue;

				switch (data[i])
				{
				case '"':
					string = !string;
					break;
				case '\'':
					string2 = !string2;
					break;
				case ' ':
					if (context.parameter_start != -1 && !context.type.size())
					{
						context.type = std::string_view(data.data() + context.parameter_start, i - context.parameter_start);
						context.parameter_start = i + 1;
					}

					break;
				case '<':
					if (data[i + 1] == '/')
						context.depth--;
					else
					{
						context.depth++;
						context.parameter_start = i + 1;
					}

					break;
				case '>':
					submit();
					break;
				}
			}

			/*
				Deal with the tail of the buffer here.
			*/

			submit();
		};

		/*
			Helper to iterate Parameter List.
		*/

		template < typename PARAMETER_CALLBACK > void Parameters(std::string_view parameter_view, PARAMETER_CALLBACK&& cb)
		{		
			bool string = false,string2 = false;
			size_t i = 0, k = 0, v = -1;

			auto submit = [&]()
			{
				if (v != -1)
				{
					bool q = parameter_view[v] == '"';

					cb(std::string_view(parameter_view.data() + k, v - 1 - k), q ? std::string_view(parameter_view.data() + v + 1, i - v - 2) : std::string_view(parameter_view.data() + v, i - v));

					k = i+1; v = -1;
				}
			};

			for (; i < parameter_view.size(); i++)
			{
				if (string && parameter_view[i] != '"')
					continue;

				if (string2 && parameter_view[i] != '\'')
					continue;

				switch (parameter_view[i])
				{
				case '"':
					string = !string;
					break;
				case '\'':
					string2 = !string2;
					break;
				case '=':
					v = i + 1;
					break;
				case ' ':
					submit();
					break;
				}
			}

			submit();
		}
	}

	namespace extrabar
	{
		/*
			Given that the extrabar is an element that has the id "extrabar" and each of the sub-elements is an anchor. Extract Title, href and img src.

			Using State Machine do the following:

			1. Find extrabar.
			2. Find anchor.
			3. Extract Parameters.
			4. Repeat 2 -> 3 until we exit extrabar.

			ON_ITEM is passed title,link and image.
		*/

		template < typename ON_ITEM > void Parse(std::string_view html, ON_ITEM && cb)
		{
			constexpr std::string_view _id("id"), _extrabar("extabar"), _a("a"),_div("div");

			enum States
			{
				find_extrabar,
				find_anchor,
				extract_details
			};

			size_t state = find_extrabar;
			size_t extrabar_depth = -1, anchor_depth = -1;

			std::string_view title, href, src;

			XML::Outline(html, [&](auto & ctx)
			{
				/*
					Completion Condition:

					We have found an extrabar, finished scanning it and are now scanning elements past it.
				*/

				if (extrabar_depth != -1 && ctx.depth <= extrabar_depth)
					return false;

				auto GetCurrentDetails = [&]()
				{
					XML::Parameters(ctx.parameters, [&](auto key, auto value)
					{
						switch (d8u::switch_t(key))
						{
						case d8u::switch_t("title"): title = value; break;
						case d8u::switch_t("href"): href = value; break;
						case d8u::switch_t("src"): src = value; break;
						}
					});
				};

				switch (state)
				{
				case find_extrabar:
					if(ctx.type == _div)
						XML::Parameters(ctx.parameters, [&](auto key, auto value)
						{
							if (key == _id && value == _extrabar)
							{
								extrabar_depth = ctx.depth;
								state = find_anchor;
							}
						});
					break;
				case extract_details:
					/*
						Element Completion Condition:

						We have found an anchor, extracted available parameters.
					*/

					GetCurrentDetails();

					if (title.size() && href.size() && src.size())
					{
						cb(title, href, src);

						state = find_anchor;
						anchor_depth = -1;

						title = std::string_view();
						href = std::string_view();
						src = std::string_view();
					}
					
					break;
				case find_anchor:
					if (ctx.type == _a)
					{
						anchor_depth = ctx.depth;
						state = extract_details;
						GetCurrentDetails();
					}
					break;
				}

				return true;
			});
		}
	}
}