!CHAPTER Module "actions" 

The action module provides the infrastructure for defining HTTP actions.

!SUBSECTION Basics

`actions.getErrorMessage(code)`

Returns the error message for an error code.

<!--

@anchor JSModuleActionsGetErrorMessage
@copydetails JSF_getErrorMessage

-->

!SECTION Standard HTTP Result Generators

`actions.resultOk(req, res, code, result, headers)`

The function defines a response. code is the status code to return. result is the result object, which will be returned as JSON object in the body. headers is an array of headers to returned. The function adds the attribute error with value false and code with value code to the result.

`actions.resultBad(req, res, error-code, msg, headers)`

The function generates an error response.

`actions.resultNotFound(req, res, code, msg, headers)`
The function generates an error response.

`actions.resultUnsupported(req, res, headers)`
The function generates an error response.

`actions.resultError(req, res, code, errorNum, errorMessage, headers, keyvals)`

The function generates an error response. The response body is an array with an attribute errorMessage containing the error message errorMessage, error containing true, code containing code, errorNum containing errorNum, and errorMessage containing the error message errorMessage. keyvals are mixed into the result.


<!--
@CLEARPAGE
@anchor JSModuleActionsResultOk
@copydetails JSF_resultOk

@CLEARPAGE
@anchor JSModuleActionsResultBad
@copydetails JSF_resultBad

@CLEARPAGE
@anchor JSModuleActionsResultNotFound
@copydetails JSF_resultNotFound

@CLEARPAGE
@anchor JSModuleActionsResultUnsupported
@copydetails JSF_resultUnsupported

@CLEARPAGE
@anchor JSModuleActionsResultError
@copydetails JSF_resultError
-->