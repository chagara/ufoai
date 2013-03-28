#pragma once

#include <string>

#include <gtk/gtktreemodel.h>
#include <gtk/gtktreeselection.h>

namespace gtkutil
{

	/**
	 * Utility class for operation on GtkTreeModels. This class provides
	 * methods to retrieve strings and other values from a tree model without
	 * having to manually use GValues etc.
	 */
	class TreeModel
	{
		public:

			/**
			 * Extract a string from the given row of the provided TreeModel.
			 *
			 * @param model
			 * The TreeModel* to examine.
			 *
			 * @param iter
			 * A GtkTreeIter pointing to the row to look up.
			 *
			 * @param colNo
			 * The column number to look up.
			 */
			static std::string getString (GtkTreeModel* model, GtkTreeIter* iter, gint colNo);

			/** Extract a boolean from the given row of the provided TreeModel.
			 *
			 * @param model
			 * The TreeModel* to examine.
			 *
			 * @param iter
			 * A GtkTreeIter pointing to the row to look up.
			 *
			 * @param colNo
			 * The column number to look up.
			 */
			static bool getBoolean (GtkTreeModel* model, GtkTreeIter* iter, gint colNo);

			/** Extract an integer from the given row of the provided TreeModel.
			 *
			 * @param model
			 * The TreeModel* to examine.
			 *
			 * @param iter
			 * A GtkTreeIter pointing to the row to look up.
			 *
			 * @param colNo
			 * The column number to look up.
			 */
			static int getInt (GtkTreeModel* model, GtkTreeIter* iter, gint colNo);

			/** Extract a pointer from the given row of the provided TreeModel.
			 *
			 * @param model
			 * The TreeModel* to examine.
			 *
			 * @param iter
			 * A GtkTreeIter pointing to the row to look up.
			 *
			 * @param colNo
			 * The column number to look up.
			 */
			static gpointer getPointer (GtkTreeModel* model, GtkTreeIter* iter, gint colNo);

			/**
			 * Extract the selected string from the given column in the TreeModel. The
			 * selection object will be queried for a selection, and the string
			 * returned if present, otherwise an empty string will be returned.
			 *
			 * @param selection
			 * GtkTreeSelection object to be tested for a selection.
			 *
			 * @param colNo
			 * The column number to extract data from if the selection is valid.
			 */
			static std::string getSelectedString (GtkTreeSelection* selection, gint colNo);

			/**
			 * Extract the selected integer from the given column in the TreeModel. The
			 * selection object will be queried for a selection, and the integer
			 * returned if present, otherwise -1 will be returned.
			 *
			 * @param selection
			 * GtkTreeSelection object to be tested for a selection.
			 *
			 * @param colNo
			 * The column number to extract data from if the selection is valid.
			 */
			static int getSelectedInt (GtkTreeSelection* selection, gint colNo);

			/**
			 * Extract the parent string from the selected entry from the given column in
			 * the TreeModel. The selection object will be queried for a selection. After that
			 * we are trying to get the parent, if there is one, return the string for the
			 * given column, if there is none an empty string will be returned.
			 *
			 * @param sel
			 * GtkTreeSelection object to be tested for a selection.
			 *
			 * @param colNo
			 * The column number to extract data from if the selection is valid.
			 */
			static std::string getSelectedParentString (GtkTreeSelection* sel, gint colNo);

			/**
			 * Extract the selected boolean from the given column in the TreeModel. The
			 * selection object will be queried for a selection.
			 *
			 * @param selection
			 * GtkTreeSelection object to be tested for a selection.
			 *
			 * @param colNo
			 * The column number to extract data from if the selection is valid.
			 *
			 * @returns: the boolean of the selected iter. If nothing is selected,
			 * the method will return false.
			 */
			static bool getSelectedBoolean (GtkTreeSelection* selection, gint colNo);

			/**
			 * Extract the selected pointer from the given column in the TreeModel. The
			 * selection object will be queried for a selection, and the pointer
			 * returned if present, otherwise @c NULL will be returned.
			 *
			 * @param selection
			 * GtkTreeSelection object to be tested for a selection.
			 *
			 * @param colNo
			 * The column number to extract data from if the selection is valid.
			 */
			static gpointer getSelectedPointer (GtkTreeSelection* selection, gint colNo);

			/**
			 * greebo: Utility callback for use in gtk_tree_view_set_search_equal_func, which
			 * enables some sort of "full string" search in treeviews.
			 *
			 * The equalFuncStringContains returns "match" as soon as the given key occurs
			 * somewhere in the string in question, not only at the beginning (GTK default).
			 *
			 * Prerequisites: The search column must contain a string.
			 */
			static gboolean equalFuncStringContains (GtkTreeModel* model, gint column, const gchar *key,
					GtkTreeIter* iter, gpointer search_data);

			/* Local object that walks the GtkTreeModel and obtains a GtkTreePath locating
			 * the given name. The gtk_tree_model_foreach function requires a pointer to
			 * a function, which in this case is a static member of the walker object that
			 * accepts a void* pointer to the instance (like other GTK callbacks).
			 */
			class SelectionFinder
			{

					// String containing the name to highlight
					std::string _selection;

					// An integer to search for (alternative to the string above)
					int _needle;

					// The GtkTreePath* pointing to the required texture
					GtkTreePath* _path;

					// The GtkTreeModel that has been searched by forEach()
					GtkTreeModel* _model;

					// The column index to be searched
					int _column;

					// TRUE, if this should search for an integer instead of a string
					bool _searchForInt;

				public:

					// Constructor to search for strings
					SelectionFinder (const std::string& selection, int column);

					// Constructor to search for integers
					SelectionFinder (int needle, int column);

					// Retrieve the found TreePath, which may be NULL if the texture was not
					// found
					GtkTreePath* getPath ();

					/** greebo: Get the GtkTreeIter corresponding to the found path.
					 * 			The returnvalue can be invalid if the internal found path is NULL.
					 */
					GtkTreeIter getIter ();

					// Static callback for GTK
					static gboolean
							forEach (GtkTreeModel* model, GtkTreePath* path, GtkTreeIter* iter, gpointer vpSelf);

			}; // class SelectionFinder

			/**
			 * greebo: Tries to lookup the given string in the given column of the given view.
			 * Returns TRUE if the lookup and the selection was successful, FALSE otherwise.
			 */
			static bool findAndSelectString(GtkTreeView* view, const std::string& needle, int column);

			/**
			 * greebo: Tries to lookup the given integer in the given column of the given view.
			 * Returns TRUE if the lookup and the selection was successful, FALSE otherwise
			 */
			static bool findAndSelectInteger(GtkTreeView* view, int needle, int column);

	};

} // namespace gtkutil
