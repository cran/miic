#' Evaluate the effective number of samples
#' @description This function evaluates the effective number of samples in a dataset.
#'
#' @param inputData [a data frame]
#' A data frame that contains the observational data. Each
#' column corresponds to one variable and each row is a sample that gives the
#' values for all the observed variables. The column names correspond to the
#' names of the observed variables. Data must be discrete like.
#' @param plot [a boolean value] if the autocorrelation plot has to be done. It will be performed only if all values of the correlation vector are positive.
#' @return A list containing the autocorrelation decay, the effective number of samples, and the result of an exponentiality test with alpha = 0.05
#' @export
#' @useDynLib miic

miic.evaluate.effn <- function(inputData = NULL, plot=T)
{
  #### Check the input arguments
  if( is.null( inputData ) )
  { stop("The input data file is required") }
  inData <- c(colnames(inputData), as.vector(as.character(t(as.matrix(inputData)))))
  if (requireNamespace("Rcpp", quietly = TRUE)) {
    res <- .Call('evaluateEffn', inData, ncol(inputData), nrow(inputData),PACKAGE = "miic")
  }
  if(length(which(res$correlation > 0)) == length(res$correlation)){

    fit1 <- MASS::fitdistr(res$correlation, "exponential")
    pval = stats::ks.test(res$correlation, "pexp", fit1$estimate)$p.value
    if(pval < 0.05){
      res$exponential_decay= FALSE
    } else {
      res$exponential_decay= TRUE
    }

    if(plot){
      graphics::plot(res$correlation, type="l", log="y", ylab="Autocorrelation with lag", xlab="n")
      graphics::title("Autocorrelation between n distant samples")
    }
  }else {
    res$exponential_decay= FALSE
  }
  res
}
