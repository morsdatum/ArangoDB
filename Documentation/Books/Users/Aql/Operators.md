!CHAPTER Operators

AQL supports a number of operators that can be used in expressions.  There are
comparison, logical, arithmetic, and the ternary operator.

!SUBSUBSECTION Comparison operators

Comparison (or relational) operators compare two operands. They can be used with
any input data types, and will return a boolean result value.

The following comparison operators are supported:

- `==` equality
- `!=` inequality
- `<`  less than 
- `<=` less or equal
- `>`  greater than
- `>=` greater or equal
- `in` test if a value is contained in a list

The `in` operator expects the second operand to be of type list. All other
operators accept any data types for the first and second operands.

Each of the comparison operators returns a boolean value if the comparison can
be evaluated and returns `true` if the comparison evaluates to true, and `false`
otherwise.

Some examples for comparison operations in AQL:

    1 > 0
    true != null
    45 <= "yikes!"
    65 != "65"
    65 == 65
    1.23 < 1.32
    1.5 IN [ 2, 3, 1.5 ]

!SUBSUBSECTION Logical operators

Logical operators combine two boolean operands in a logical operation and return
a boolean result value.

The following logical operators are supported:

- `&&` logical and operator
- `||` logical or operator
- `!` logical not/negation operator

Some examples for logical operations in AQL:

    u.age > 15 && u.address.city != ""
    true || false
    !u.isInvalid

The `&&`, `||`, and `!` operators expect their input operands to be boolean
values each. If a non-boolean operand is used, the operation will fail with an
error. In case all operands are valid, the result of each logical operator is a
boolean value.

Both the `&&` and `||` operators use short-circuit evaluation and only evaluate
the second operand if the result of the operation cannot be determined by
checking the first operand alone.

!SUBSUBSECTION Arithmetic operators

Arithmetic operators perform an arithmetic operation on two numeric
operands. The result of an arithmetic operation is again a numeric value.
Operators are supported.

AQL supports the following arithmetic operators:

- `+` addition
- `-` subtraction
- `*` multiplication
- `/` division
- `%` modulus

These operators work with numeric operands only. Invoking any of the operators
with non-numeric operands will result in an error. An error will also be raised
for some other edge cases as division by zero, numeric over- or underflow etc.
If both operands are numeric and the computation result is also valid, the
result will be returned as a numeric value.

The unary plus and unary minus are supported as well.

Some example arithmetic operations:

    1 + 1
    33 - 99
    12.4 * 4.5
    13.0 / 0.1
    23 % 7
    -15
    +9.99

!SUBSUBSECTION Ternary operator

AQL also supports a ternary operator that can be used for conditional
evaluation. The ternary operator expects a boolean condition as its first
operand, and it returns the result of the second operand if the condition
evaluates to true, and the third operand otherwise.

Example:

    u.age > 15 || u.active == true ? u.userId : null

!SUBSUBSECTION Range operator

AQL supports expressing simple numeric ranges with the `..` operator.
This operator can be used to easily iterate over a sequence of numeric
values.    

The `..` operator will produce a list of values in the defined range, with 
both bounding values included.

Example:

    2010..2013

will produce the following result:

    [ 2010, 2011, 2012, 2013 ]

!SUBSUBSECTION Operator precedence

The operator precedence in AQL is as follows (lowest precedence first):

- `? :` ternary operator
- `||` logical or
- `&&` logical and
- `==`, `!=` equality and inequality
- `in` in operator
- `<`, `<=`, `>=`, `>` less than, less equal,
  greater equal, greater than
- `+`, `-` addition, subtraction
- `*`, `/`, `%` multiplication, division, modulus
- `!`, `+`, `-` logical negation, unary plus, unary minus
- `[*]` expansion
- `()` function call
- `.` member access
- `[]` indexed value access

The parentheses `(` and `)` can be used to enforce a different operator
evaluation order.

!SUBSECTION Functions

AQL supports functions to allow more complex computations. Functions can be
called at any query position where an expression is allowed. The general
function call syntax is:

    FUNCTIONNAME(arguments)

where `FUNCTIONNAME` is the name of the function to be called, and `arguments`
is a comma-separated list of function arguments. If a function does not need any
arguments, the argument list can be left empty. However, even if the argument
list is empty the parentheses around it are still mandatory to make function
calls distinguishable from variable names.

Some example function calls:

    HAS(user, "name")
    LENGTH(friends)
    COLLECTIONS()

In contrast to collection and variable names, function names are case-insensitive, 
i.e. `LENGTH(foo)` and `length(foo)` are equivalent.

!SUBSUBSECTION Extending AQL
 
Since ArangoDB 1.3, it is possible to extend AQL with user-defined functions. 
These functions need to be written in Javascript, and be registered before usage
in a query.

Please refer to [Extending AQL](../ExtendingAql/README.md) for more details on this.

By default, any function used in an AQL query will be sought in the built-in 
function namespace `_aql`. This is the default namespace that contains all AQL
functions that are shipped with ArangoDB. 
To refer to a user-defined AQL function, the function name must be fully qualified
to also include the user-defined namespace. The `::` symbol is used as the namespace
separator:

    MYGROUP::MYFUNC()

    MYFUNCTIONS::MATH::RANDOM()
    
As all AQL function names, user function names are also case-insensitive.


!SUBSUBSECTION Type cast functions

As mentioned before, some of the operators expect their operands to have a
certain data type. For example, the logical operators expect their operands to
be boolean values, and the arithmetic operators expect their operands to be
numeric values.  If an operation is performed with operands of an unexpected type,
the operation will fail with an error. To avoid such failures, value types can
be converted explicitly in a query. This is called type casting.

In an AQL query, type casts are performed only upon request and not implicitly.
This helps avoiding unexpected results. All type casts have to be performed by
invoking a type cast function. AQL offers several type cast functions for this
task. Each of the these functions takes an operand of any data type and returns
a result value of type corresponding to the function name (e.g. `TO_NUMBER()`
will return a number value):

- *TO_BOOL(value)*: Takes an input *valu*e of any type and converts it 
  into the appropriate boolean value as follows:
  - `null` is converted to `false`.
  - Numbers are converted to `true` if they are unequal to 0, and to `false` otherwise. 
  - Strings are converted to `true` if they are non-empty, and to `false` otherwise. 
  - Lists are converted to `true` if they are non-empty, and to `false` otherwise.
  - Documents are converted to `true` if they are non-empty, and to `false` otherwise.

- *TO_NUMBER(value)*: Takes an input *value* of any type and converts it 
  into a numeric value as follows:
  - `null`, `false`, lists, and documents are converted to the value `0`.
  - `true` is converted to `1`.
  - Strings are converted to their numeric equivalent if the full string content is
    is a valid number, and to `0` otherwise.

- *TO_STRING(value)*: Takes an input *value* of any type and converts it 
  into a string value as follows:
  - `null` is converted to the string `"null"`
  - `false` is converted to the string `"false"`, `true` to the string `"true"`
  - Numbers, lists and documents are converted to their string equivalents. 

- *TO_LIST(value)*: Takes an input *value* of any type and converts it 
  into a list value as follows:
  - `null` is converted to an empty list
  - Boolean values, numbers and strings are converted to a list containing the original
    value as its single element
  - Documents are converted to a list containing their attribute values as list elements

!SUBSUBSECTION Type check functions

AQL also offers functions to check the data type of a value at runtime. The
following type check functions are available. Each of these functions takes an
argument of any data type and returns true if the value has the type that is
checked for, and false otherwise.

The following type check functions are available:

- *IS_NULL(value)*: Checks whether *value* is a `null` value

- *IS_BOOL(value)*: Checks whether *value* is a `boolean` value

- *IS_NUMBER(value)*: Checks whether *value* is a `numeric` value

- *IS_STRING(value)*: Checks whether *value* is a `string` value

- *IS_LIST(value)*: Checks whether *value* is a `list` value

- *IS_DOCUMENT(value)*: Checks whether *value* is a `document` value

!SUBSUBSECTION String functions

For string processing, AQL offers the following functions:

- *CONCAT(value1, value2, ... valuen)*: Concatenate the strings 
  passed as in *value1* to *valuen*. `null` values are ignored

- *CONCAT_SEPARATOR(separator, value1, value2, ... valuen)*: 
  Concatenate the strings passed as arguments *value1* to *valuen* using the 
  *separator* string. `null` values are ignored

- *CHAR_LENGTH(value)*: Return the number of characters in *value*. This is
  a synonym for LENGTH(value)*

- *LOWER(value)*: Lower-case *value*

- *UPPER(value)*: Upper-case *value*

- *SUBSTRING(value, offset, length)*: Return a substring of *value*,
  starting at @FA{offset} and with a maximum length of *length* characters. Offsets
  start at position 0

- *LEFT(value, LENGTH)*: Returns the *LENGTH* leftmost characters of
  the string *value*

- *RIGHT(value, LENGTH)*: Returns the *LENGTH* rightmost characters of
  the string *value*

- *TRIM(value, type)*: Returns the string *value* with whitespace stripped 
  from the start and/or end. The optional *type* parameter specifies from which parts
  of the string the whitespace is stripped:
  - *type* 0 will strip whitespace from the start and end of the string
  - *type* 1 will strip whitespace from the start of the string only
  - *type* 2 will strip whitespace from the end of the string only

- *REVERSE(value)*: Returns the reverse of the string *value*

- *CONTAINS(text, search, return-index)*: Checks whether the string
  *search* is contained in the string *text*. By default, this function returns 
  `true` if *search* is contained in *text*, and `false` otherwise. By
  passing `true` as the third function parameter *return-index*, the function
  will return the position of the first occurrence of *search* within *text*, 
  starting at offset 0, or `-1` if *search* is not contained in *text*.

  The string matching performed by *CONTAINS* is case-sensitive.

- *LIKE(text, search, case-insensitive)*: Checks whether the pattern
  *search* is contained in the string *text*, using wildcard matching. 
  Returns `true` if the pattern is contained in *text*, and `false` otherwise. 
  The *pattern* string can contain the wildcard characters `%` (meaning any
  sequence of characters) and `_` (any single character).

  The string matching performed by *LIKE* is case-sensitive by default, but by
  passing `true` as the third parameter, the matching will be case-insensitive.

  The value for *search* cannot be a variable or a document attribute. The actual 
  value must be present at query parse time already.

!SUBSUBSECTION Numeric functions

AQL offers some numeric functions for calculations. The following functions are
supported:

- *FLOOR(value)*: Returns the integer closest but not greater to *value*

- *CEIL(value)*: Returns the integer closest but not less than *value*

- *ROUND(value)*: Returns the integer closest to *value*

- *ABS(value)*: Returns the absolute part of *value*

- *SQRT(value)*: Returns the square root of *value*

- *RAND()*: Returns a pseudo-random number between 0 and 1

!SUBSUBSECTION Date functions

AQL offers functionality to work with dates. Dates are no datatypes of their own in 
AQL (neither they are in JSON, which is often used as a format to ship data into and
out of ArangoDB). Instead, dates in AQL are internally represented by either numbers 
(timestamps) or strings. The date functions in AQL provide mechanisms to convert from 
a numeric timestamp to a string representation and vice versa. 

There are two date functions in AQL to create dates for further use:

- *DATE_TIMESTAMP(date)*: Creates a UTC timestamp value from *date*. 

- *DATE_TIMESTAMP(year, month, day, hour, minute, second, millisecond)*: 
  Same as before, but allows specifying the individual date components separately.
  All parameters after *day* are optional.

- *DATE_ISO8601(date)*: Returns an ISO8601 date time string from *date*.
  The date time string will always use UTC time, indicated by the `Z` at its end.

- *DATE_ISO8601(year, month, day, hour, minute, second, millisecond)*: 
  same as before, but allows specifying the individual date components separately.
  All parameters after *day* are optional.

These two above date functions accept the following input values:

- numeric timestamps, indicating the number of milliseconds elapsed since the UNIX
  epoch (i.e. January 1st 1970 00:00:00 UTC).
  An example timestamp value is `1399472349522`, which translates to 
  `2014-05-07T14:19:09.522Z`.

- date time strings in formats *YYYY-MM-DDTHH:MM:SS.MMM*, 
  *YYYY-MM-DD HH:MM:SS.MMM*, or *YYYY-MM-DD* Milliseconds are always optional.
  A timezone difference may optionally be added at the end of the string, with the
  hours and minutes that need to be added or subtracted to the date time value.
  For example, `2014-05-07T14:19:09+01:00` can be used to specify a one hour offset,
  and `2014-05-07T14:19:09+07:30` can be specified for seven and half hours offset. 
  Negative offsets are also possible. Alternatively to an offset, a `Z` can be used
  to indicate UTC / Zulu time. 
 
  An example value is `2014-05-07T14:19:09.522Z` meaning May 7th 2014, 14:19:09 and 
  522 milliseconds, UTC / Zulu time. Another example value without time component is 
  `2014-05-07Z`.

  Please note that if no timezone offset is specified in a datestring, ArangoDB will
  assume UTC time automatically. This is done to ensure portability of queries across
  servers with different timezone settings, and because timestamps will always be
  UTC-based. 

- individual date components as separate function arguments, in the following order:
  - year 
  - month
  - day
  - hour
  - minute
  - second
  - millisecond

  All components following `day` are optional and can be omitted. Note that no
  timezone offsets can be specified when using separate date components, and UTC /
  Zulu time will be used.
 
The following calls to `DATE_TIMESTAMP` are equivalent and will all return 
`1399472349522`:

    DATE_TIMESTAMP("2014-05-07T14:19:09.522")
    DATE_TIMESTAMP("2014-05-07T14:19:09.522Z")
    DATE_TIMESTAMP("2014-05-07 14:19:09.522")
    DATE_TIMESTAMP("2014-05-07 14:19:09.522Z")
    DATE_TIMESTAMP(2014, 5, 7, 14, 19, 9, 522)
    DATE_TIMESTAMP(1399472349522)

The same is true for calls to `DATE_ISO8601` that also accepts variable input 
formats:

    DATE_ISO8601("2014-05-07T14:19:09.522Z")
    DATE_ISO8601("2014-05-07 14:19:09.522Z")
    DATE_ISO8601(2014, 5, 7, 14, 19, 9, 522)
    DATE_ISO8601(1399472349522)

The above functions are all equivalent and will return `"2014-05-07T14:19:09.522Z"`.

The following date functions can be used with dates created by `DATE_TIMESTAMP` and
`DATE_ISO8601`:

- *DATE_DAYOFWEEK(date)*: Returns the weekday number of *date*. The
  return values have the following meanings:
  - 0: Sunday
  - 1: Monday
  - 2: Tuesday
  - 3: Wednesday
  - 4: Thursday
  - 5: Friday
  - 6: Saturday

- *DATE_YEAR(date)*: Returns the year part of *date* as a number. 

- *DATE_MONTH(date)*: Returns the month part of *date* as a number.

- *DATE_DAY(date)*: Returns the day part of *date* as a number. 

- *DATE_HOUR(date)*: Returns the hour part of *date* as a number. 

- *DATE_MINUTE(date)*: Returns the minute part of *date* as a number. 

- *DATE_SECOND(date)*: Returns the seconds part of *date* as a number. 

- *DATE_MILLISECOND(date)*: Returns the milliseconds part of *date* as a number. 

The following other date functions are also available:

- *DATE_NOW()*: Returns the current time as a timestamp.
  
  Note that this function is evaluated on every invocation and may return different 
  values when invoked multiple times in the same query.

!SUBSUBSECTION List functions

AQL supports the following functions to operate on list values:

- *LENGTH(list)*: Returns the length (number of list elements) of *list*. If 
  *list* is a document, returns the number of attribute keys of the document, 
  regardless of their values.

- @FN{FLATTEN(list), depth)*: Turns a list of lists into a flat list. All 
  list elements in *list* will be expanded in the result list. Non-list elements 
  are added as they are. The function will recurse into sub-lists up to a depth of
  *depth*. *depth* has a default value of 1.

  Example:
  
      FLATTEN([ 1, 2, [ 3, 4 ], 5, [ 6, 7 ], [ 8, [ 9, 10 ] ])

  will produce:

      [ 1, 2, 3, 4, 5, 6, 7, 8, [ 9, 10 ] ]

  To fully flatten the list, use a *depth* of 2:
      
      FLATTEN([ 1, 2, [ 3, 4 ], 5, [ 6, 7 ], [ 8, [ 9, 10 ] ], 2)

  This will produce:
      
      [ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 ]

- *MIN(list)*: Returns the smallest element of *list*. `null` values
  are ignored. If the list is empty or only `null` values are contained in the list, the
  function will return `null`.

- *MAX(list)*: Returns the greatest element of *list*. `null` values
  are ignored. If the list is empty or only `null` values are contained in the list, the
  function will return `null`.

- *AVERAGE(list)*: Returns the average (arithmetic mean) of the values in *list*. 
  This requires the elements in *list* to be numbers. `null` values are ignored. 
  If the list is empty or only `null` values are contained in the list, the function 
  will return `null`.

- *SUM(list)*: Returns the sum of the values in *list*. This
  requires the elements in *list* to be numbers. `null` values are ignored. 

- *MEDIAN(list)*: Returns the median value of the values in *list*. This 
  requires the elements in *list* to be numbers. `null` values are ignored. If the 
  list is empty or only `null` values are contained in the list, the function will return 
  `null`.

- *VARIANCE_POPULATION(list)*: Returns the population variance of the values in 
  *list*. This requires the elements in *list* to be numbers. `null` values 
  are ignored. If the list is empty or only `null` values are contained in the list, 
  the function will return `null`.

- *VARIANCE_SAMPLE(list)*: Returns the sample variance of the values in 
  *list*. This requires the elements in *list* to be numbers. `null` values 
  are ignored. If the list is empty or only `null` values are contained in the list, 
  the function will return `null`.

- *STDDEV_POPULATION(list)*: Returns the population standard deviation of the 
  values in *list*. This requires the elements in *list* to be numbers. `null` 
  values are ignored. If the list is empty or only `null` values are contained in the list, 
  the function will return `null`.

- *STDDEV_SAMPLE(list)*: Returns the sample standard deviation of the values in 
  *list*. This requires the elements in *list* to be numbers. `null` values 
  are ignored. If the list is empty or only `null` values are contained in the list, 
  the function will return `null`.

- *REVERSE(list)*: Returns the elements in *list* in reversed order.

- *FIRST(list)*: Returns the first element in *list* or `null` if the
  list is empty.

- *LAST(list)*: Returns the last element in *list* or `null` if the
  list is empty.

- *NTH(list, position)*: Returns the list element at position @FA{position}.
  Positions start at 0. If *position* is negative or beyond the upper bound of the list
  specified by *list*, then `null` will be returned.

- *POSITION(list, search, return-index)*: Returns the position of the
  element *search* in list *list*. Positions start at 0. If the element is not 
  found, then `-1` is returned. If *return-index* is `false`, then instead of the
  position only `true` or `false` are returned, depending on whether the sought element
  is contained in the list.

- *SLICE(list, start, length)*: Extracts a slice of the list specified
  by *list*. The extraction will start at list element with position *start*. 
  Positions start at 0. Up to *length* elements will be extracted. If *length* is
  not specified, all list elements starting at @FA{start} will be returned.
  If *start* is negative, it can be used to indicate positions from the end of the
  list.

  Examples:

      SLICE([ 1, 2, 3, 4, 5 ], 0, 1)
      
  will return `[ 1 ]`
  
      SLICE([ 1, 2, 3, 4, 5 ], 1, 2)
      
  will return `[ 2, 3 ]`
  
      SLICE([ 1, 2, 3, 4, 5 ], 3) 
  
  will return `[ 4, 5 ]`
  
      SLICE([ 1, 2, 3, 4, 5 ], 1, -1) 
      
  will return `[ 2, 3, 4 ]`
  
      SLICE([ 1, 2, 3, 4, 5 ], 0, -2)
      
  will return `[ 1, 2, 3 ]`

- *UNIQUE(list)*: Returns all unique elements in *list*. To determine
  uniqueness, the function will use the comparison order defined in @ref AqlTypeOrder.
  Calling this function might return the unique elements in any order.

- *UNION(list1, list2, ...)*: Returns the union of all lists specified.
  The function expects at least two list values as its arguments. The result is a list
  of values in an undefined order.

  Note: No duplicates will be removed. In order to remove duplicates, please use either
  *UNION_DISTINCT* function or apply the *UNIQUE* on the result of *union*.

  Example:

      RETURN UNION(
        [ 1, 2, 3 ],
        [ 1, 2 ]
      )

  will produce:

      [ [ 1, 2, 3, 1, 2 ] ]

  with duplicate removal:

      RETURN UNIQUE(
        UNION(
          [ 1, 2, 3 ],
          [ 1, 2 ]
        )
      )
  
  will produce:

      [ [ 1, 2, 3 ] ]

- *UNION_DISTINCT(list1, list2, ...)*: Returns the union of distinct values of
  all lists specified. The function expects at least two list values as its arguments. 
  The result is a list of values in an undefined order.

- *MINUS(list1, list2, ...)*: Returns the difference of all lists specified.
  The function expects at least two list values as its arguments.
  The result is a list of values that occur in the first list but not in any of the
  subsequent lists. The order of the result list is undefined and should not be relied on.
  Note: duplicates will be removed.

- *INTERSECTION(list1, list2, ...)*: Returns the intersection of all lists specified.
  The function expects at least two list values as its arguments.
  The result is a list of values that occur in all arguments. The order of the result list
  is undefined and should not be relied on.
  
  Note: Duplicates will be removed.


Apart from these functions, AQL also offers several language constructs (e.g.
`FOR`, `SORT`, `LIMIT`, `COLLECT`) to operate on lists.

!SUBSUBSECTION Document functions

AQL supports the following functions to operate on document values:

- *MATCHES(document, examples, return-index)*: Compares the document
  *document* against each example document provided in the list *examples*. 
  If *document* matches one of the examples, `true` is returned, and if there is
  no match `false` will be returned. The default return value type can be changed by
  passing `true` as the third function parameter *return-index*. Setting this
  flag will return the index of the example that matched (starting at offset 0), or 
  `-1` if there was no match.

  The comparisons will be started with the first example. All attributes of the example
  will be compared against the attributes of *document*. If all attributes match, the 
  comparison stops and the result is returned. If there is a mismatch, the function will
  continue the comparison with the next example until there are no more examples left.

  The *examples* must be a list of 1..n example documents, with any number of attributes
  each. Note: specifying an empty list of examples is not allowed.
   
  Example usage:

      RETURN MATCHES(
        { "test" : 1 }, [ 
          { "test" : 1, "foo" : "bar" }, 
          { "foo" : 1 }, 
          { "test : 1 } 
        ], true)

  This will return `2`, because the third example matches, and because the 
  `return-index` flag is set to `true`.

- *MERGE(document1, document2, ... *documentn)*: Merges the documents
  in *document1* to *documentn* into a single document. If document attribute
  keys are ambiguous, the merged result will contain the values of the documents 
  contained later in the argument list.

  For example, two documents with distinct attribute names can easily be merged into one: 

      RETURN MERGE(
        { "user1" : { "name" : "J" } }, 
        { "user2" : { "name" : "T" } }
      )

      [ 
        { "user1" : { "name" : "J" }, 
    "user2" : { "name" : "T" } }
      ]

  When merging documents with identical attribute names, the attribute values of the
  latter documents will be used in the end result:

      RETURN MERGE(
        { "users" : { "name" : "J" } }, 
        { "users" : { "name" : "T" } }
      )

      [ 
        { "users" : { "name" : "T" } } 
      ]

  Please note that merging will only be done for top-level attributes. If you wish to
  merge sub-attributes, you should consider using `MERGE_RECURSIVE` instead.

- *MERGE_RECURSIVE(document1, document2, ... documentn)*: Recursively
  merges the documents in *document1* to *documentn* into a single document. If 
  document attribute keys are ambiguous, the merged result will contain the values of the 
  documents contained later in the argument list.

  For example, two documents with distinct attribute names can easily be merged into one: 

      RETURN MERGE_RECURSIVE(
        { "user-1" : { "name" : "J", "livesIn" : { "city" : "LA" } } }, 
        { "user-1" : { "age" : 42, "livesIn" : { "state" : "CA" } } }
      )

      [ 
        { "user-1" : { "name" : "J", "livesIn" : { "city" : "LA", "state" : "CA" }, "age" : 42 } } 
      ]


- *HAS(document, attributename)*: Returns `true` if *document* has an
  attribute named *attributename*, and `false` otherwise.

- *ATTRIBUTES(document, *removeInternal, sort)*: Returns the attribute
  names of the document *document as a list. 
  If *removeInternal* is set to `true`, then all internal attributes (such as `_id`, 
  `_key` etc.) are removed from the result. If *sort* is set to `true`, then the
  attribute names in the result will be sorted. Otherwise they will be returned in any order.

- *UNSET(document, attributename, ...)*: Removes the attributes *attributename*
  (can be one or many) from *document*. All other attributes will be preserved.
  Multiple attribute names can be specified by either passing multiple individual string argument 
  names, or by passing a list of attribute names:

      RETURN UNSET(doc, '_id', '_key', [ 'foo', 'bar' ])

- *KEEP(document, attributename, ...)*: Keeps only the attributes *attributename*
  (can be one or many) from *document*. All other attributes will be removed from the result.
  Multiple attribute names can be specified by either passing multiple individual string argument 
  names, or by passing a list of attribute names:

      RETURN KEEP(doc, 'firstname', 'name', 'likes')

- *PARSE_IDENTIFIER(document-handle)*: Parses the document handle specified in 
  *document-handle* and returns a the handle's individual parts a separate attributes.
  This function can be used to easily determine the collection name and key from a given document.
  The *document-handle* can either be a regular document from a collection, or a document
  identifier string (e.g. `_users/1234`). Passing either a non-string or a non-document or a
  document without an `_id` attribute will result in an error.

      RETURN PARSE_IDENTIFIER('_users/my-user')

      [ 
        { "collection" : "_users", "key" : "my-user" } 
      ]

      RETURN PARSE_IDENTIFIER({ "_id" : "mycollection/mykey", "value" : "some value" })

      [ 
        { "collection" : "mycollection", "key" : "mykey" } 
      ]

!SUBSUBSECTION Geo functions

AQL offers the following functions to filter data based on geo indexes:

- *NEAR(collection, latitude, longitude, limit, distancename)*: 
  Returns at most *limit* documents from collection *collection* that are near
  *latitude* and *longitude*. The result contains at most *limit* documents, returned in
  any order. If more than *limit* documents qualify, it is undefined which of the qualifying
  documents are returned. Optionally, the distances between the specified coordinate
  (*latitude* and *longitude*) and the document coordinates can be returned as well.
  To make use of that, an attribute name for the distance result has to be specified in
  the *distancename* argument. The result documents will contain the distance value in
  an attribute of that name.
  *limit* is an optional parameter since ArangoDB 1.3. If it is not specified or null, a limit
  value of 100 will be applied.

- *WITHIN(collection, latitude, longitude, radius, distancename)*: 
  Returns all documents from collection *collection* that are within a radius of
  *radius* around that specified coordinate (*latitude* and *longitude*). The order
  in which the result documents are returned is undefined. Optionally, the distance between the
  coordinate and the document coordinates can be returned as well.
  To make use of that, an attribute name for the distance result has to be specified in
  the *distancename* argument. The result documents will contain the distance value in
  an attribute of that name.

Note: these functions require the collection *collection* to have at least
one geo index.  If no geo index can be found, calling this function will fail
with an error.

!SUBSUBSECTION Fulltext functions

AQL offers the following functions to filter data based on fulltext indexes:

- *FULLTEXT(collection, attribute, query)*: 
  Returns all documents from collection *collection* for which the attribute *attribute*
  matches the fulltext query *query*.
  *query* is a comma-separated list of sought words (or prefixes of sought words). To 
  distinguish between prefix searches and complete-match searches, each word can optionally be
  prefixed with either the `prefix:` or `complete:` qualifier. Different qualifiers can
  be mixed in the same query. Not specifying a qualifier for a search word will implicitly
  execute a complete-match search for the given word:

  - `FULLTEXT(emails, "body", "banana")` Will look for the word `banana` in the 
    attribute `body` of the collection `collection`.

  - `FULLTEXT(emails, "body", "banana,orange")` Will look for boths the words 
    `banana` and `orange` in the mentioned attribute. Only those documents will be
    returned that contain both words.

  - `FULLTEXT(emails, "body", "prefix:head")` Will look for documents that contain any
    words starting with the prefix `head`.

  - `FULLTEXT(emails, "body", "prefix:head,complete:aspirin")` Will look for all 
    documents that contain a word starting with the prefix `head` and that also contain 
    the (complete) word `aspirin`. Note: specifying `complete` is optional here.

  - `FULLTEXT(emails, "body", "prefix:cent,prefix:subst")` Will look for all documents 
    that contain a word starting with the prefix `cent` and that also contain a word
    starting with the prefix `subst`.

  If multiple search words (or prefixes) are given, then by default the results will be 
  AND-combined, meaning only the logical intersection of all searches will be returned. 
  It is also possible to combine partial results with a logical OR, and with a logical NOT:

  - `FULLTEXT(emails, "body", "+this,+text,+document")` Will return all documents that 
    contain all the mentioned words. Note: specifying the `+` symbols is optional here.

  - `FULLTEXT(emails, "body", "banana,|apple")` Will return all documents that contain
    either (or both) words `banana` or `apple`.

  - `FULLTEXT(emails, "body", "banana,-apple")` Will return all documents that contain
    the word `banana` but do not contain the word `apple`.

  - `FULLTEXT(emails, "body", "banana,pear,-cranberry")` Will return all documents that 
    contain both the words `banana` and `pear` but do not contain the word 
    `cranberry`.

  No precedence of logical operators will be honored in a fulltext query. The query will simply
  be evaluated from left to right.
  
Note: the `FULLTEXT` function requires the collection *collection* to have a
fulltext index on `attribute`. If no fulltext index is available, this function
will fail with an error.

!SUBSUBSECTION Graph functions

AQL has the following functions to traverse graphs:

- *PATHS(vertexcollection, edgecollection, direction, followcycles)*: 
  returns a list of paths through the graph defined by the nodes in the collection 
  *vertexcollection* and edges in the collection *edgecollection*. For each vertex
  in *vertexcollection*, it will determine the paths through the graph depending on the
  value of @FA{direction}:
  - `"outbound"`: Follow all paths that start at the current vertex and lead to another vertex
  - `"inbound"`: Follow all paths that lead from another vertex to the current vertex
  - `"any"`: Combination of `"outbound"` and `"inbound"`
  The default value for *direction* is `"outbound"`.
  If *followcycles* is true, cyclic paths will be followed as well. This is turned off by
  default.

  The result of the function is a list of paths. Paths of length 0 will also be returned. Each 
  path is a document consisting of the following attributes:
  - `vertices`: list of vertices visited along the path
  - `edges`: list of edges visited along the path (might be empty)
  - `source`: start vertex of path
  - `destination`: destination vertex of path

  Example calls:

      PATHS(friends, friendrelations, "outbound", false)

      FOR p IN PATHS(friends, friendrelations, "outbound") 
        FILTER p.source._id == "123456/123456" && LENGTH(p.edges) == 2
        RETURN p.vertices[*].name

- *TRAVERSAL(vertexcollection, edgecollection, startVertex, direction, options)*: 
  Traverses the graph described by *vertexcollection* and *edgecollection*, 
  starting at the vertex identified by id *startVertex*. Vertex connectivity is
  specified by the *direction* parameter:
  - `"outbound"`: Vertices are connected in `_from` to `_to` order
  - `"inbound"`: Vertices are connected in `_to` to `_from` order
  - `"any"`: Vertices are connected in both `_to` to `_from` and in 
    `_from` to `_to` order

  Additional options for the traversal can be provided via the *options* document:
  - `strategy`: Defines the traversal strategy. Possible values are `depthfirst` 
    and `breadthfirst`. Defaults to `depthfirst`
  - `order`: Defines the traversal order: Possible values are `preorder` and
    `postorder`. Defaults to `preorder`
  - `itemOrder`: Defines the level item order. Can be `forward` or 
    `backward`. Defaults to `forward`
  - `minDepth`: Minimum path depths for vertices to be included. This can be used to
    include only vertices in the result that are found after a certain minimum depth.
    Defaults to 0 
  - `maxIterations`: Maximum number of iterations in each traversal. This number can be
    set to prevent endless loops in traversal of cyclic graphs. When a traversal performs
    as many iterations as the `maxIterations` value, the traversal will abort with an
    error. If `maxIterations` is not set, a server-defined value may be used
  - `maxDepth`: Maximum path depth for sub-edges expansion. This can be used to 
    limit the depth of the traversal to a sensible amount. This should especially be used
    for big graphs to limit the traversal to some sensible amount, and for graphs 
    containing cycles to prevent infinite traversals. The maximum depth defaults to 256, 
    with the chance of this value being non-sensical. For several graphs, a much lower
    maximum depth is sensible, whereas for other, more list-oriented graphs a higher
    depth should be used
  - `paths`: If `true`, the paths encountered during the traversal will
    also be returned along with each traversed vertex. If `false`, only the 
    encountered vertices will be returned.
  - `uniqueness`: An optional document with the following attributes:
    - `vertices`: 
      - `none`: No vertex uniqueness is enforced
      - `global`: A vertex may be visited at most once. This is the default.
      - `path`: A vertex is visited only if not already contained in the current
        traversal path
    - `edges`: 
      - `none`: No edge uniqueness is enforced
      - `global`: An edge may be visited at most once. This is the default
      - `path`: An edge is visited only if not already contained in the current
        traversal path
  - `followEdges`: An optional list of example edge documents that the traversal will
    expand into. If no examples are given, the traversal will follow all edges. If one
    or many edge examples are given, the traversal will only follow an edge if it matches
    at least one of the specified examples. `followEdges` can also be a string with the
    name of an AQL user-defined function that should be responsible for checking if an
    edge should be followed. In this case, the AQL function will is expected to have the
    following signature:

        function (config, vertex, edge, path)

    The function is expected to return a boolean value. If it returns `true`, the edge
    will be followed. If `false` is returned, the edge will be ignored.

  - `filterVertices`: An optional list of example vertex documents that the traversal will
    treat specially. If no examples are given, the traversal will handle all encountered
    vertices equally. If one or many vertex examples are given, the traversal will exclude
    any non-matching vertex from the result and/or not descend into it. Optionally,
    `filterVertices` can contain the name of a user-defined AQL function that should be responsible
    for filtering. If so, the AQL function is expected to have the following signature:

        function (config, vertex, path)

    If a custom AQL function is used, it is expected to return one of the following values:
    - `[ ]`: Include the vertex in the result and descend into its connected edges
    - `[ "prune" ]`: Will include the vertex in the result but not descend into its connected edges
    - `[ "exclude" ]`: Will not include the vertex in the result but descend into its connected edges
    - `[ "prune", "exclude" ]`: Will completely ignore the vertex and its connected edges

  - `vertexFilterMethod`: Only useful in conjunction with `filterVertices` and if no user-defined
    AQL function is used. If specified, it will influence how vertices are handled that don't match 
    the examples in `filterVertices`:
    - `[ "prune" ]`: Will include non-matching vertices in the result but not descend into them
    - `[ "exclude" ]`: Will not include non-matching vertices in the result but descend into them
    - `[ "prune", "exclude" ]`: Will neither include non-matching vertices in the result nor descend into them

  The result of the TRAVERSAL function is a list of traversed points. Each point is a 
  document consisting of the following attributes:
  - `vertex`: The vertex at the traversal point
  - `path`: The path history for the traversal point. The path is a document with the
    attributes `vertices` and `edges`, which are both lists. Note that `path` is only present
    in the result if the `paths` attribute is set in the @FA{options}

  Example calls:

      TRAVERSAL(friends, friendrelations, "friends/john", "outbound", {
        strategy: "depthfirst",
        order: "postorder",
        itemOrder: "backward",
        maxDepth: 6,
        paths: true
      })

      // filtering on specific edges (by specifying example edges)
      TRAVERSAL(friends, friendrelations, "friends/john", "outbound", {
        strategy: "breadthfirst",
        order: "preorder",
        itemOrder: "forward",
        followEdges: [ { type: "knows" }, { state: "FL" } ]
      })

      // filtering on specific edges and vertices
      TRAVERSAL(friends, friendrelations, "friends/john", "outbound", {
        strategy: "breadthfirst",
        order: "preorder",
        itemOrder: "forward",
        followEdges: [ { type: "knows" }, { state: "FL" } ],
        filterVertices: [ { isActive: true }, { isDeleted: false } ],
        vertexFilterMethod: [ "prune", "exclude" ]
      })

      // using user-defined AQL functions for edge and vertex filtering
      TRAVERSAL(friends, friendrelations, "friends/john", "outbound", {
        followEdges: "myfunctions::checkedge",
        filterVertices: "myfunctions::checkvertex"
      })

      // to register the custom AQL functions, execute something in the fashion of the 
      // following commands in arangosh once: 
      var aqlfunctions = require("org/arangodb/aql/functions");

      // these are the actual filter functions
      aqlfunctions.register("myfunctions::checkedge", function (config, vertex, edge, path) { 
        return (edge.type !== 'dislikes'); // don't follow these edges
      }, false);

      aqlfunctions.register("myfunctions::checkvertex", function (config, vertex, path) { 
        if (vertex.isDeleted || ! vertex.isActive) {
          return [ "prune", "exclude" ]; // exclude these and don't follow them
        }
        return [ ]; // include everything else
      }, false);

- *TRAVERSAL_TREE(vertexcollection, edgecollection, startVertex, direction, connectName, options)*: 
  Traverses the graph described by *vertexcollection* and *edgecollection*, 
  starting at the vertex identified by id @FA{startVertex} and creates a hierarchical result.
  Vertex connectivity is establish by inserted an attribute which has the name specified via
  the *connectName* parameter. Connected vertices will be placed in this attribute as a 
  list.

  The *options* are the same as for the `TRAVERSAL` function, except that the result will
  be set up in a way that resembles a depth-first, pre-order visitation result. Thus, the
  `strategy` and `order` attributes of the *options* attribute will be ignored.

  Example calls:

      TRAVERSAL_TREE(friends, friendrelations, "friends/john", "outbound", "likes", { 
        itemOrder: "forward"
      })

When using one of AQL's graph functions please make sure that the graph does not contain cycles,
or that you at least specify some maximum depth or uniqueness criteria for a traversal. 

If no bounds are set, a traversal might run into an endless loop in a cyclic graph or sub-graph,
and even in a non-cyclic graph, traversing far into the graph might consume a lot of processing
time and memory for the result set.

- *SHORTEST_PATH(vertexcollection, edgecollection, startVertex, endVertex, direction, options)*: 
  Determines the first shortest path from the @FA{startVertex} to the *endVertex*.
  Both vertices must be present in the vertex collection specified in *vertexcollection*,
  and any connecting edges must be present in the collection specified by *edgecollection*.
  Vertex connectivity is specified by the @FA{direction} parameter:
  - `"outbound"`: Vertices are connected in `_from` to `_to` order
  - `"inbound"`: Vertices are connected in `_to` to `_from` order
  - `"any"`: Vertices are connected in both `_to` to `_from` and in 
    `_from` to `_to` order
  The search is aborted when a shortest path is found. Only the first shortest path will be
  returned. Any vertex will be visited at most once by the search.

  Additional options for the traversal can be provided via the *options* document:
  - `maxIterations`: Maximum number of iterations in the search. This number can be
    set to bound long-running searches. When a search performs as many iterations as the 
    `maxIterations` value, the search will abort with an error. If `maxIterations` is not 
    set, a server-defined value may be used.
  - `paths`: If `true`, the result will not only contain the vertices along the shortest
    path, but also the connecting edges. If `false`, only the encountered vertices will 
    be returned.
  - `distance`: An optional custom function to be used when calculating the distance 
    between a vertex and a neighboring vertex. The expected function signature is:

        function (config, vertex1, vertex2, edge)

    Both vertices and the connecting edge will be passed into the function. The function
    is expected to return a numeric value that expresses the distance between the two
    vertices. Higher values will mean higher distances, giving the connection a lower
    priority in further analysis.
    If no custom distance function is specified, all vertices are assumed to have the
    same distance (1) to each other. If a function name is specified, it must have been
    registered as a regular user-defined AQL function.

  - `followEdges`: An optional list of example edge documents that the search will
    expand into. If no examples are given, the search will follow all edges. If one
    or many edge examples are given, the search will only follow an edge if it matches
    at least one of the specified examples. `followEdges` can also be a string with the
    name of an AQL user-defined function that should be responsible for checking if an
    edge should be followed. In this case, the AQL function will is expected to have the
    following signature:

        function (config, vertex, edge, path)

    The function is expected to return a boolean value. If it returns `true`, the edge
    will be followed. If `false` is returned, the edge will be ignored.

  - `filterVertices`: An optional list of example vertex documents that the search will
    treat specially. If no examples are given, the search will handle all encountered
    vertices equally. If one or many vertex examples are given, the search will exclude
    the vertex from the result and/or not descend into it. Optionally, `filterVertices` can 
    contain the name of a user-defined AQL function that should be responsible for filtering.
    If so, the AQL function is expected to have the following signature:

        function (config, vertex, path)

    If a custom AQL function is used, it is expected to return one of the following values:
    - `[ ]`: Include the vertex in the result and descend into its connected edges
    - `[ "prune" ]`: Will include the vertex in the result but not descend into its connected edges
    - `[ "exclude" ]`: Will not include the vertex in the result but descend into its connected edges
    - `[ "prune", "exclude" ]`: Will completely ignore the vertex and its connected edges

  The result of the SHORTEST_PATH function is a list with the components of the shortest
  path. Each component is a document consisting of the following attributes:
  - `vertex`: The vertex at the traversal point
  - `path`: The path history for the traversal point. The path is a document with the
    attributes `vertices` and `edges`, which are both lists. Note that `path` is only present
    in the result if the `paths` attribute is set in the @FA{options}. 

  Example calls:

      SHORTEST_PATH(cities, motorways, "cities/CGN", "cities/MUC", "outbound", {
        paths: true
      })

      // using a user-defined distance function
      SHORTEST_PATH(cities, motorways, "cities/CGN", "cities/MUC", "outbound", {
        paths: true,
        distance: "myfunctions::citydistance"
      })

      // using a user-defined function to filter edges
      SHORTEST_PATH(cities, motorways, "cities/CGN", "cities/MUC", "outbound", {
        paths: true,
        followEdges: "myfunctions::checkedge"
      })

      // to register a custom AQL distance function, execute something in the fashion of the 
      // following commands in arangosh once: 
      var aqlfunctions = require("org/arangodb/aql/functions");

      // this is the actual distance function
      aqlfunctions.register("myfunctions::distance", function (config, vertex1, vertex2, edge) { 
        return Math.sqrt(Math.pow(vertex1.x - vertex2.x) + Math.pow(vertex1.y - vertex2.y));
      }, false);

      // this is the filter function for the edges
      aqlfunctions.register("myfunctions::checkedge", function (config, vertex, edge, path) { 
        return (edge.underConstruction === false); // don't follow these edges
      }, false);

- *EDGES(edgecollection, startvertex, direction, edgeexamples)*:
  Return all edges connected to the vertex *startvertex* as a list. The possible values for
  *direction* are:
  - `outbound`: Return all outbound edges
  - `inbound`: Return all inbound edges
  - `any`: Return outbound and inbound edges
  
  The *edgeexamples* parameter can optionally be used to restrict the results to specific
  edge connections only. The matching is then done via the *MATCHES* function.
  To not restrict the result to specific connections, *edgeexamples* should be left
  unspecified. 

  Example calls:
 
      EDGES(friendrelations, "friends/john", "outbound")
      EDGES(friendrelations, "friends/john", "any", [ { "$label": "knows" } ])

- *NEIGHBORS(vertexcollection, edgecollection, startvertex, direction, edgeexamples)*:
  Return all neighbors that are directly connected to the vertex *startvertex* as a list. 
  The possible values for *direction* are:
  - `outbound`: Return all outbound edges
  - `inbound`: Return all inbound edges
  - `any`: Return outbound and inbound edges

  The *edgeexamples* parameter can optionally be used to restrict the results to specific
  edge connections only. The matching is then done via the *MATCHES* function.
  To not restrict the result to specific connections, *edgeexamples* should be left
  unspecified.

  Example calls:

      NEIGHBORS(friends, friendrelations, "friends/john", "outbound")
      NEIGHBORS(users, usersrelations, "users/john", "any", [ { "$label": "recommends" } ] )

!SUBSUBSECTION Control flow functions

AQL offers the following functions to let the user control the flow of operations:

- *NOT_NULL(alternative, ...)*: Returns the first alternative that is not `null`, 
  and `null` if all alternatives are `null` themselves

- *FIRST_LIST(alternative, ...)*: Returns the first alternative that is a list, and
  `null` if none of the alternatives is a list

- *FIRST_DOCUMENT(alternative, ...)*: Returns the first alternative that is a document,
  and `null` if none of the alternatives is a document

!SUBSUBSECTION Miscellaneous functions

Finally, AQL supports the following functions that do not belong to any of the other
function categories:

- *COLLECTIONS()*: Returns a list of collections. Each collection is returned as a document
  with attributes `name` and `_id`

- *CURRENT_USER()*: Returns the name of the current user. The current user is the user 
  account name that was specified in the `Authorization` HTTP header of the request. It will
  only be populated if authentication on the server is turned on, and if the query was executed
  inside a request context. Otherwise, the return value of this function will be `null`.

- *DOCUMENT(collection, id)*: Returns the document which is uniquely identified by
  the *id*. ArangoDB will try to find the document using the `_id` value of the document
  in the specified collection. If there is a mismatch between the *collection* passed and
  the collection specified in *id*, then `null` will be returned. Additionally, if the
  *collection* matches the collection value specified in *id* but the document cannot be
  found, `null` will be returned. This function also allows *id* to be a list of ids.
  In this case, the function will return a list of all documents that could be found. 

  Examples:
      
      DOCUMENT(users, "users/john")
      DOCUMENT(users, "john")

      DOCUMENT(users, [ "users/john", "users/amy" ])
      DOCUMENT(users, [ "john", "amy" ])

  Note: The *DOCUMENT* function is polymorphic since ArangoDB 1.4. It can now be used with
  a single parameter *id* as follows:

- *DOCUMENT(id)*: In this case, *id* must either be a document handle string
  (consisting of collection name and document key) or a list of document handle strings, e.g.

      DOCUMENT("users/john")
      DOCUMENT([ "users/john", "users/amy" ])

- *SKIPLIST(collection, condition, skip, limit)*: Return all documents 
  from a skiplist index on collection *collection* that match the specified @FA{condition}.
  This is a shortcut method to use a skiplist index for retrieving specific documents in 
  indexed order. The skiplist index supports equality and less than/greater than queries. The
  @FA{skip} and *limit* parameters are optional but can be specified to further limit the
  results:

      SKIPLIST(test, { created: [[ '>', 0 ]] }, 0, 100)
      SKIPLIST(test, { age: [[ '>', 25 ], [ '<=', 65 ]] })
      SKIPLIST(test, { a: [[ '==', 10 ]], b: [[ '==', 25 ]] }

  The *condition* document must contain an entry for each attribute that is contained in the
  index. It is not allowed to specify just a subset of attributes that are present in an index.
  Additionally the attributes in the *condition* document must be specified in the same order 
  as in the index. 
  If no suitable skiplist index is found, an error will be raised and the query will be aborted.
