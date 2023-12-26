#include "Header.h"

namespace AST {

    auto operator<< (std::ostream& o, const AST::BinOpID id) -> std::ostream& {
        switch (id) {
            case AST::BinOpID::And:                 o << "&"; break;
            case AST::BinOpID::Equivalence:         o << "<->"; break;
            case AST::BinOpID::Implication:         o << "->"; break;
            case AST::BinOpID::Nand:                o << "!&"; break;
            case AST::BinOpID::NImplication:        o << "!->"; break;
            case AST::BinOpID::Nor:                 o << "!|"; break;
            case AST::BinOpID::NReverseImplication: o << "!<-"; break;
            case AST::BinOpID::Or:                  o << "|"; break;
            case AST::BinOpID::ReverseImplication:  o << "<-"; break;
            case AST::BinOpID::Xor:                 o << "^"; break;
        }
        return o;
    }

    auto operator<< (std::ostream& o, const AST::BinOp op) -> std::ostream& {
        o << "(" << op.lhs << " " << op.id << " " << op.rhs << ")";
        return o;
    }

    auto operator<< (std::ostream& o, const AST::UnOpID id) -> std::ostream& {
        switch (id) {
            case AST::UnOpID::Not: o << "!"; break;
            default: break;
        }
        return o;
    }

    auto operator<< (std::ostream& o, const AST::UnOp op) -> std::ostream& {
        o << op.id << op.rhs;
        return o;
    }

    auto operator<< (std::ostream& o, const AST::lformula_t& formula) -> std::ostream& {
        //std::cout << "printing" << std::endl;
        boost::apply_visitor(boost::hana::overload(
                    [&](const auto& arg) -> void {
                        o << arg;
                    }
            ), formula);
        return o;
    }

    auto operator<< (std::ostream& o, const AST::statement_t& statement) -> std::ostream& {
        boost::apply_visitor(boost::hana::overload(
            [&](const auto& arg) -> void {
                o << arg;
                }
                ), statement);
        return o;
    }

    auto operator<< (std::ostream& o, const RuleInvocation& invocation) -> std::ostream& {
        o << invocation.name << "(";
        std::string sep = "";
        for (const auto& arg: invocation.arguments) {
            o << sep << arg;
            sep = ", ";
        }
        o << ")";
        return o;
    }

    auto operator<< (std::ostream& o, const Definition& definition) -> std::ostream& {
        o << definition.name << " = " << definition.value << " : " << definition.reasoning;
        return o;
    }

    auto to_string(const statement_t& statement, int indentation) -> std::string {
        std::stringstream result;
        boost::apply_visitor(boost::hana::overload(
            [&](const Hypothesis& arg) -> void {
                result << to_string(arg, indentation);
                },
            [&](const Definition& arg) -> void {
                result << arg;
                }
                ), statement);
        result << ";";
        return result.str();
    }

    auto operator<< (std::ostream& o, const AST::Hypothesis& hypothesis) -> std::ostream& {
        o << to_string(hypothesis, 0);
        return o;
    }

    auto to_string(const Hypothesis& hypothesis, int indentation) -> std::string {
        std::stringstream result;

        result << "hypothesis " << hypothesis.condition_name << " = " << hypothesis.condtition << " {\n";
        for (const auto& statement: hypothesis.statements)
            result << std::string((indentation + 1) * 4, ' ') 
                << to_string(statement, indentation + 1) << "\n";
        result << std::string((indentation) * 4, ' ') << "}";
        
        return result.str();
    }

    auto operator<< (std::ostream& o, const RuleBlock& rule) -> std::ostream& {
        o << "rule " << rule.name << "(";
        std::string sep = "";
        for (const auto& arg: rule.arguments) {
            o << sep << arg;
            sep = ", ";
        }
        o << ") => ";
        sep = "";
        for (const auto& transformat: rule.transformats) {
            o << sep << transformat;
            sep = ", ";
        }
        o << " {\n";
        for (const auto& statement: rule.statements)
            o << std::string(4, ' ') << to_string(statement, 1) << "\n";
        o << "}";
        return o;
    }
    
    auto operator<< (std::ostream& o, const AxiomBlock& axiom) -> std::ostream& {
        o << "axiom " << axiom.name << "(";
        std::string sep = "";
        for (const auto& arg: axiom.arguments) {
            o << sep << arg;
            sep = ", ";
        }
        o << ") => ";
        sep = "";
        for (const auto& transformat: axiom.transformats) {
            o << sep << transformat;
            sep = ", ";
        }
        o << ";";
        return o;
    }

    auto equal_symbolic(const lformula_t& lhs, const lformula_t& rhs) -> bool {
        //std::cout << lhs << " , " << rhs << std::endl;
        return boost::apply_visitor(boost::hana::overload(
            [&](const std::string& lhs_, const std::string& rhs_) -> bool { return lhs_ == rhs_; },
            [&](const BinOp& lhs_, const BinOp& rhs_) -> bool {
                if (lhs_.id != rhs_.id) return false;
                return equal_symbolic(lhs_.lhs, rhs_.lhs) && equal_symbolic(lhs_.rhs, rhs_.rhs);
                },
            [&](const UnOp& lhs_, const UnOp& rhs_) -> bool {
                if (lhs_.id != rhs_.id) return false;
                return equal_symbolic(lhs_.rhs, rhs_.rhs);
                },
            [&](const auto& lhs_, const auto& rhs_) -> bool { return false; }
        ), lhs, rhs);
    }

    auto operator<< (std::ostream& o, const block_t& block) -> std::ostream& {
        return boost::apply_visitor(boost::hana::overload( 
            [&](const auto& arg) -> std::ostream& { o << arg; return o; }
        ), block);
    }
}
