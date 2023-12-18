// -*- c-basic-offset: 4 -*-
/** @file ImageVariable.h
 *
 *  @author James Legg
 * 
 *  @brief Define & Implement the ImageVariable class
 *
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _PANODATA_IMAGEVARIABLE_H
#define _PANODATA_IMAGEVARIABLE_H
#include <memory>


namespace HuginBase
{

/** An ImageVariable stores a value that can be linked to other ImageVariables
 * of the same type.
 *
 * When you link ImageVariables, setting one using setData sets all the other
 * linked ImageVariables. Then when reading the ImageVariables data with getData
 * you get the last value set on any of the linked ImageVariables.
 * 
 * @tparam Type The type of the data value to store with this variable. It is
 * the return type for getData and the parameter type for setData.
 * 
 * @todo These will probably be copied when creating the Undo/redo data, but
 * their pointers will remain the same. Therefore when copying a SrcPanoImg, we
 * should offset all the pointers by the difference between the old and new
 * SrcPanoImg, so the links are relatively the same.
 */
template <class Type>
class ImageVariable
{
public:
    /// constructor
    ImageVariable();
    
    /// construct with a given initial data value
    explicit ImageVariable(Type data);
    
    /** Construct linked with something else.
     * 
     * Sets the data from the linked item. Behaves like linkWith.
     * 
     * @see linkWith
     * @param link ImageVariable which to link with. It provides the data for
     * the newly constructed variable.
     */
    explicit ImageVariable(ImageVariable<Type> * link);
    
    /** Copy constructor
     * 
     * Generally copied for a getSrcImage call, so make the copy independant from
     * the other image variables.
     */
    ImageVariable(const ImageVariable &source);
    
    /// destructor
    ~ImageVariable();
    
    /// get the variable's value
    Type getData() const;
    
    /** Set the variable groups' value.
     * 
     * Calling this sets the value returned by getData for this ImageVariable
     * and any ImageVariables linked to this one.
     * 
     * @param data the data to be set for this group.
     */
    void setData(const Type data);
    
    /** Create a link.
     * 
     * After making a link to another variable, this variable, and any variables
     * we link link to, will now have the passed in variable's data. Calling
     * setData(data) on either variable, or any variable previously linked with
     * either variable, sets the return value for both variable's getData()
     * function, and the getData function of variables previously linked with
     * either variable.
     * (i.e. Links are bidirectional and accumlative)
     * 
     * @param link a pointer to the variable to link with.
     */
    void linkWith(ImageVariable<Type> * link);
    
    /** remove all links
     * 
     * After calling this, setData will only change the value of this
     * ImageVariable.
     */
    void removeLinks();
    
    /** Find out if there are other linked variables.
     * 
     * @return true if there are any other variables linked with this one,
     * false otherwise.
     */
    bool isLinked() const;
    
    /** Find out if this variable is linked to a given variable
     * 
     * Find if this variable has been linked (either directly or indirectly)
     * with the variable pointed to by otherVariable.
     * 
     * @param otherVariable the variable to check linkage with.
     * 
     * @return true if this variable is linked with otherVariable,
     * false otherwise.
     */
    bool isLinkedWith(const ImageVariable<Type> * otherVariable) const;
    
protected:
    std::shared_ptr<Type> m_ptr;
}; // ImageVariable class

/////////////////////////////
// Public member functions //
/////////////////////////////

// Constructors

template <class Type>
ImageVariable<Type>::ImageVariable() : m_ptr(new Type())
{
}

template <class Type>
ImageVariable<Type>::ImageVariable(Type data) : m_ptr(new Type(data))
{
}

template <class Type>
ImageVariable<Type>::ImageVariable(ImageVariable<Type> * link) : m_ptr(link->m_ptr)
{
}

template <class Type>
ImageVariable<Type>::ImageVariable(const ImageVariable<Type> & source) : m_ptr(new Type(*source.m_ptr))
{
}

// Destructor

template <class Type>
ImageVariable<Type>::~ImageVariable()
{
}

// Other public member functions

template <class Type>
Type ImageVariable<Type>::getData() const
{
    return *m_ptr;
}

template <class Type>
void ImageVariable<Type>::setData(const Type data)
{
    *m_ptr = data;
}

template <class Type>
void ImageVariable<Type>::linkWith(ImageVariable<Type> * link)
{
    // We need to first check that we aren't linked already.
    if (m_ptr == link->m_ptr)
    {
        DEBUG_INFO("Attempt to link already linked variables");
        return;
    }
    else
    {

        m_ptr = link->m_ptr;
    }
}

template <class Type>
void ImageVariable<Type>::removeLinks()
{
    m_ptr.reset(new Type(*m_ptr));
}

template <class Type>
bool ImageVariable<Type>::isLinked() const
{
    return m_ptr.use_count() > 1;
}

template <class Type>
bool ImageVariable<Type>::isLinkedWith(const ImageVariable<Type> * otherVariable) const
{
    // return true if we can find a link with the given item.
    return m_ptr == otherVariable->m_ptr;
}

} // HuginBase namespace

#endif // ndef _PANODATA_IMAGEVARIABLE_H
