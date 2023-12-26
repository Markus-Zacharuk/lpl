#ifndef HEADER_H
#define HEADER_H

/*
#define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
#define BOOST_MPL_LIMIT_VECTOR_SIZE 50                
#define BOOST_MPL_LIMIT_MAP_SIZE 50 

#define BOOST_MPL_LIMIT_LIST_SIZE 50
#define BOOST_VARIANT_VISITATION_UNROLLING_LIMIT 50
*/
#include "Includes.h"

extern char* yytext;
extern unsigned int line;
extern FILE *yyin;
extern bool from_file;

extern int dbg_counter;
#define DBG(TYPE1, TYPE2, MSG) \
    std::cout << dbg_counter++ << "\n" << (TYPE1) << "! " << (TYPE2) << ": " << (MSG) << "\n" \
        << __FILE__ << ":" << __LINE__ << ": " << __PRETTY_FUNCTION__ << "\n\n";

#define DBGQ(TYPE1, TYPE2, MSG, QUOTE) \
    std::cout << dbg_counter++ << "\n" << (TYPE1) << "! " << (TYPE2) << ": " << (MSG) << "\n### CODE ###\n" << (QUOTE) << "\n### CODE END ###\n" \
        << __FILE__ << ":" << __LINE__ << ": " << __PRETTY_FUNCTION__ << "\n\n";


namespace AST {    

    enum class UnOpID {
        Not
    };
    enum class BinOpID {
        And, Nand,
        Or, Xor, Nor,
        Implication, ReverseImplication, NImplication, NReverseImplication,
        Equivalence
    };
    template <typename T> struct UnOpTemplate {
        UnOpID id;
        T rhs;
        UnOpTemplate(UnOpID id, const T& rhs) : id(id), rhs(rhs) {};
    };
    template <typename T> struct BinOpTemplate {
        BinOpID id;
        T lhs, rhs;
        BinOpTemplate(BinOpID id, const T& lhs, const T& rhs) : id(id), lhs(lhs), rhs(rhs) {};
    };

    // https://www.boost.org/doc/libs/1_56_0/doc/html/variant/tutorial.html
    typedef boost::make_recursive_variant <
        std::string,
        UnOpTemplate<boost::recursive_variant_>,
        BinOpTemplate<boost::recursive_variant_>
    //    std::vector<boost::recursive_variant_>
    >::type lformula_t;

    using UnOp = UnOpTemplate<lformula_t>;
    using BinOp = BinOpTemplate<lformula_t>;

    struct Equal {
        std::string lhs;
        lformula_t rhs;
        Equal() {}
        Equal(const std::string& lhs, const lformula_t& rhs): lhs(lhs), rhs(rhs) {}        
    };

    struct RuleInvocation {
        std::string name;
        std::vector<std::string> arguments;
        RuleInvocation() {};
        RuleInvocation(const std::string& arg) : name(arg){};
        RuleInvocation(const std::string& arg1, const std::string& arg2) : name(arg1), arguments({arg2}) {};
        void add(const std::string& arg) { arguments.push_back(arg); }
    };

    struct Definition {
        std::string name;
        lformula_t value;
        RuleInvocation reasoning;
        Definition(const std::string &name, const lformula_t& value, const RuleInvocation& reasoning) : 
            name(name), value(value), reasoning(reasoning) {}
    };

    template <typename T> struct HypothesisTemplate {
        lformula_t condtition;
        std::string condition_name;
        std::vector<T> statements;
        HypothesisTemplate() {}
        HypothesisTemplate(const std::string& name, const lformula_t& cond) : condtition(cond), condition_name(name) {}
        void add_statement(const T& arg) { statements.push_back(arg); }
    };

    typedef boost::make_recursive_variant <
        HypothesisTemplate<boost::recursive_variant_>,
        Definition
    >::type statement_t;

    auto to_string(const statement_t& statement, int indentation=0) -> std::string;

    using Hypothesis = HypothesisTemplate<statement_t>;

    struct RuleBlock {
        std::string name;
        std::vector<lformula_t> arguments;
        std::vector<lformula_t> transformats;
        std::vector<statement_t> statements;
        RuleBlock() {}
        RuleBlock(const std::string& name) : name(name) {}
        void add_argument(const lformula_t& arg) { arguments.push_back(arg); }
        void add_transformat(const lformula_t& arg) { transformats.push_back(arg); }
        void add_statement(const statement_t& arg) { statements.push_back(arg); }
    };
    
    struct AxiomBlock {
        std::string name;
        std::vector<lformula_t> arguments;
        std::vector<lformula_t> transformats;
        AxiomBlock() {}
        AxiomBlock(const std::string& name) : name(name) {}
        void add_argument(const lformula_t& arg) { arguments.push_back(arg); }
        void add_transformat(const lformula_t& arg) { transformats.push_back(arg); }
    };

    using block_t = boost::variant<RuleBlock, AxiomBlock>;
    using file_t = std::vector<block_t>;

    auto operator<< (std::ostream& o, const BinOpID id)                 -> std::ostream&;
    auto operator<< (std::ostream& o, const BinOp op)                   -> std::ostream&;
    auto operator<< (std::ostream& o, const lformula_t& formula)        -> std::ostream&;
    auto operator<< (std::ostream& o, const RuleInvocation& invocation) -> std::ostream&;
    auto operator<< (std::ostream& o, const Definition& definition)     -> std::ostream&;
    auto operator<< (std::ostream& o, const statement_t& statement)     -> std::ostream&;
    auto to_string(const statement_t& statement, int indentation)       -> std::string;
    auto operator<< (std::ostream& o, const Hypothesis& hypothesis)     -> std::ostream&;
    auto to_string(const Hypothesis& hypothesis, int indentation)       -> std::string;
    auto operator<< (std::ostream& o, const RuleBlock& rule)            -> std::ostream&;
    auto operator<< (std::ostream& o, const AxiomBlock& axiom)          -> std::ostream&;
    auto operator<< (std::ostream& o, const block_t& block)               -> std::ostream&;

    auto equal_symbolic(const lformula_t& lhs, const lformula_t& rhs) -> bool;
}

namespace Eval
{
    struct Rule {
        std::vector<AST::lformula_t> transformats;
        std::vector<AST::lformula_t> arguments;
    };

    struct Axiom {
        std::vector<AST::lformula_t> transformats;
        std::vector<AST::lformula_t> arguments;
    };

    using shared = std::shared_ptr<int>;
    using name_value_t = boost::variant<Rule, Axiom>; //, AST::lformula_t>;
    using names_map_t = std::unordered_map<std::string, name_value_t>;
    struct Context {
        bool warn_redefinition = true;
        names_map_t names;
    };
    using local_names_map_t = std::unordered_map<std::string, AST::lformula_t>;

    auto operator<< (std::ostream& o, const Rule& rule) -> std::ostream&;
    auto operator<< (std::ostream& o, const Axiom& axiom) -> std::ostream&;
    auto operator<< (std::ostream& o, const name_value_t& value) -> std::ostream&;
    auto operator<< (std::ostream& o, const names_map_t& names) -> std::ostream&;
    auto operator<< (std::ostream& o, const local_names_map_t& names) -> std::ostream&;


    auto eval(const AST::AxiomBlock& axiom, Context& context) -> bool;
    auto verify(const AST::Hypothesis& hypothesis, const Context& context, local_names_map_t& names) -> bool;
    auto resolve_global_name(const std::string& name, const Context& context) -> const Eval::name_value_t*;
    template <typename T>
    auto  resolve_global_typed(const std::string& name, const Context& context) -> const T*;
    auto resolve_local_name(const std::string& name, const local_names_map_t& names) -> const AST::lformula_t*;
    auto verify_transformation(
        const AST::lformula_t& lhs, const AST::lformula_t& rhs, std::unordered_map<std::string, std::string>& name_pattern) 
        -> bool;
    auto transform(const AST::lformula_t& to, const AST::lformula_t& from, const Context& context, local_names_map_t& names) 
        -> std::optional<AST::lformula_t>;
    auto verify_by_transformation(
        const Rule& rule, const std::vector<std::string>& arguments, const AST::lformula_t& result, 
        const Context& context, local_names_map_t& names) 
        -> bool;
    auto verify_by_transformation(
        const Axiom& axiom, const std::vector<std::string>& arguments, const AST::lformula_t& result, 
        const Context& context, local_names_map_t& names) 
        -> bool;
    auto verify(const AST::Definition& definition, const Context& context, local_names_map_t& names) -> bool;
    auto verify(const AST::RuleBlock& rule, const Context& context) -> bool;
    auto eval(const AST::RuleBlock& rule, Context& context) -> bool;
    auto eval(const AST::block_t& block, Context& context) -> bool;
    auto eval(const AST::file_t& file, Context& context) -> bool;
    
} // namespace Eval


//#include "location.hh"
#include "parser.tab.hpp"

#endif
