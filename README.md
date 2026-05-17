## Description
I wanted to get a quick entry pass into the world of programming language hacking, so I decided to read Crafting Interpreters by Robert Nystorm. Seeing his Java implementation of
the Tree Walk interpreter in the first half of the book, I figured I could make it a bit more challenging for myself by writing the whole thing in C. Note that for the most part I 
have only referred to the grammar of Lox specified in the book. The implementation is my own, handwritten code. I admit however, that I referred to the implementation of the 
Hashtable rather heavily, and in the initial stages the implementation for the parser(I couldn't wrap my head around Recursive Descent parsing for a short while).

## Features(so far)
- Variable declaration and assignment in the global scope
- If statements
- Basic arithmetic/logical expression evaluations

## Planned
The scope of my implementation as mentioned is only the imperative aspect of Lox. So I am not delving into the OOP aspects of it as of yet. That in mind, things that need to be done:
- Variable declaration and assignment in nested scopes(Some infrastructure is there, but needs testing)
- `while` and `for` loops
- Functions(Maybe closures too)

## References
Thanks to Robert Nystorm for providing a free reference: https://craftinginterpreters.com

