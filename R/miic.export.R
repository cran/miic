#' Export miic result to different plotting methods
#'
#' @description This function creates an object built from the result returned
#' by \code{\link{miic}} that is ready to be fed to different plotting methods.
#'
#' @details See the details of specific function for each method.
#' For igraph, see \code{\link{getIgraph}}.
#'
#' @param miic.res [a miic graph object]
#' The graph object returned by the miic execution.
#' @param method A string representing the plotting method.
#' Currently only "igraph" is supported.
#'
#' @export
#'
#' @return A graph object adapted to the method.
#'
#' @seealso
#' \code{\link{getIgraph}} for details on the igraph exported object.
#'
#' @examples
#' \donttest{
#' library(miic)
#' data(hematoData)
#'
#' # execute MIIC (reconstruct graph)
#' miic.res <- miic(
#'   input_data = hematoData, latent = "yes",
#'   n_shuffles = 10, conf_threshold = 0.001
#' )
#'
#' # Using igraph
#' if(require(igraph)) {
#' g = miic.export(miic.res, "igraph")
#' plot(g) # Default visualisation, calls igraph::plot.igraph()
#'
#' # Specifying layout (see ?igraph::layout_)
#' l <-layout_with_kk(g)
#' plot(g, layout=l)
#'
#' # Override some graphical parameters
#' plot(g, edge.curved = .2)
#' plot(g, vertex.shape="none", edge.color="gray85", vertex.label.color="gray10")
#' }
#'
#' }
#'

miic.export <- function(miic.res, method = NULL) {
  if (is.null(miic.res$all.edges.summary)) {
    stop("The inferred network does not exist")
  }
  if (is.null(method)) {
    stop("Plotting method is required")
  }
  if (method == "igraph") {
    return(getIgraph(miic.res))
  } else {
    stop("Method not supported")
  }
}


#' Igraph plotting function for miic
#'
#' @description This functions returns an igraph object built from the result
#' returned by \code{\link{miic}}.
#'
#' @details
#' Edges attributes are passed to the igraph graph and can be accessed with
#' e.g. \code{E(g)$partial_correlation}. See \code{\link{miic}} for more
#' details on edge parameters. By default, edges are colored according to the
#' partial correlation between two nodes conditioned on the conditioning set
#' (negative is blue, null is gray and positive is red) and their width is
#' based on the conditional mutual information minus the complexity cost.
#'
#' @param miic.res [a miic graph object]
#' The graph object returned by the miic execution.
#'
#' @return An igraph graph object.
#'
#' @seealso
#' \code{\link{miic}} for details on edge parameters in the returned object,
#' \code{\link[igraph]{igraph.plotting}} for the detailed description of the
#' plotting parameters and \code{\link[igraph]{layout}} for different layouts.
#'
#'

getIgraph <- function(miic.res) {
  if (is.null(miic.res$all.edges.summary)) {
    stop("The inferred network does not exist.")
  }
  if (!base::requireNamespace("igraph", quietly = TRUE)) {
    stop("Package 'igraph' is required.")
  }

  summary = miic.res$all.edges.summary[miic.res$all.edges.summary$type %in% c('P', 'TP', 'FP'), ]
  # Re-order summary so that all edges go from "x" to "y"
  for(row in 1:nrow(summary)){
    if(summary[row, "infOrt"] == -2){
      summary[row, c("x","y")] = summary[row, c("y","x")]
      summary[row, "infOrt"] = 2
      if(!is.na(summary[row, "proba"])){
        summary[row, "proba"] = paste0(rev(
          strsplit(summary[row, "proba"], ";")[[1]]), collapse=";")
      }
      if(!is.na(summary[row, "trueOrt"])){
        summary[row, "trueOrt"] = 2
      }
    }
  }
  # Create igraph object from summary
  ig_graph = igraph::graph_from_data_frame(summary)

  # Set correct orientations
  igraph::E(ig_graph)$arrow.mode = rep(0, igraph::gsize(ig_graph))
  igraph::E(ig_graph)$arrow.mode[igraph::E(ig_graph)$infOrt == 2]  = 2
  igraph::E(ig_graph)$arrow.mode[igraph::E(ig_graph)$infOrt == -2] = 1
  igraph::E(ig_graph)$arrow.mode[igraph::E(ig_graph)$infOrt == 6]  = 3

  # Set visuals
  igraph::V(ig_graph)$color <- "lightblue"
  igraph::V(ig_graph)$label.cex <- 0.8
  igraph::V(ig_graph)$size <- 12

  min_width = 0.2
  igraph::E(ig_graph)$width <-
    pmax(log10(igraph::E(ig_graph)$info_cond - igraph::E(ig_graph)$cplx),
         min_width)
  igraph::E(ig_graph)$arrow.size <- scales::rescale(igraph::E(ig_graph)$width, to=c(0.2,1))

  # Negative pcors are blue, null is dark grey and positive are red
  igraph::E(ig_graph)$color <- "darkgray"
  pcor_palette = grDevices::colorRampPalette(c("blue", "darkgrey", "red"))
  edge_colors_indices = sapply(
    igraph::E(ig_graph)$partial_correlation,
    function(pcor) {
      ifelse(is.na(pcor), 100, abs(round(pcor * 100)) + 100 * (pcor > 0))
    }
  )
  igraph::E(ig_graph)$color <- pcor_palette(200)[edge_colors_indices]

  return(ig_graph)
}
