// Copyright 2019 Pokitec
// All rights reserved.

#pragma once

#include "ASTExpression.hpp"
#include "TTauri/utils.hpp"
#include <vector>

namespace TTauri::Config {

struct ASTExpressions : ASTNode {
    std::vector<ASTExpression *> expressions;

    ASTExpressions(Location location, ASTExpression *firstExpression) : ASTNode(location), expressions({firstExpression}) {}

    ~ASTExpressions() {
        for (auto expression: expressions) {
            delete expression;
        }
    }
};

}
