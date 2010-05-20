<?xml version="1.0"?>

<!--
/*
 *  A template to generate wrapper classes for [XP]COM interfaces (defined
 *  in XIDL) to use them in the main Qt-based GUI in platform-independent
 *  script-like manner.
 *
 *  The generated header requires COMDefs.h and must be included from there.
 */

/*
     Copyright (C) 2006-2008 Oracle Corporation

     This file is part of VirtualBox Open Source Edition (OSE), as
     available from http://www.virtualbox.org. This file is free software;
     you can redistribute it and/or modify it under the terms of the GNU
     General Public License (GPL) as published by the Free Software
     Foundation, in version 2 as it comes in the "COPYING" file of the
     VirtualBox OSE distribution. VirtualBox OSE is distributed in the
     hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */
-->

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="text"/>

<xsl:strip-space elements="*"/>


<!--
//  helper definitions
/////////////////////////////////////////////////////////////////////////////
-->

<!--
 *  capitalizes the first letter
-->
<xsl:template name="capitalize">
  <xsl:param name="str" select="."/>
  <xsl:value-of select="
    concat(
      translate(substring($str,1,1),'abcdefghijklmnopqrstuvwxyz','ABCDEFGHIJKLMNOPQRSTUVWXYZ'),
      substring($str,2)
    )
  "/>
</xsl:template>

<!--
 *  uncapitalizes the first letter only if the second one is not capital
 *  otherwise leaves the string unchanged
-->
<xsl:template name="uncapitalize">
  <xsl:param name="str" select="."/>
  <xsl:choose>
    <xsl:when test="not(contains('ABCDEFGHIJKLMNOPQRSTUVWXYZ', substring($str,2,1)))">
      <xsl:value-of select="
        concat(
          translate(substring($str,1,1),'ABCDEFGHIJKLMNOPQRSTUVWXYZ','abcdefghijklmnopqrstuvwxyz'),
          substring($str,2)
        )
      "/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="string($str)"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!--
 *  translates the string to uppercase
-->
<xsl:template name="uppercase">
  <xsl:param name="str" select="."/>
  <xsl:value-of select="
    translate($str,'abcdefghijklmnopqrstuvwxyz','ABCDEFGHIJKLMNOPQRSTUVWXYZ')
  "/>
</xsl:template>


<!--
//  templates
/////////////////////////////////////////////////////////////////////////////
-->


<!--
 *  shut down all implicit templates
-->
<xsl:template match="*"/>
<xsl:template match="*|/" mode="declare"/>
<xsl:template match="*|/" mode="define"/>
<xsl:template match="*|/" mode="end"/>
<xsl:template match="*|/" mode="begin"/>


<!--
 *  header
-->
<xsl:template match="/idl">

<xsl:text>
/*
 *  DO NOT EDIT! This is a generated file.
 *
 *  Qt-based wrapper classes for VirtualBox Main API (COM interfaces)
 *  generated from XIDL (XML interface definition).
 *
 *  Source    : src/VBox/Main/idl/VirtualBox.xidl
 *  Generator : src/VBox/Frontends/VirtualBox/include/COMWrappers.xsl
 *
 *  Note: this header must be included from COMDefs.h, never directly.
 */
</xsl:text>

<!-- all enum declarations -->
<xsl:text>
// all enums

</xsl:text>
  <xsl:for-each select="*/enum">
    <xsl:text>enum </xsl:text>
    <xsl:value-of select="concat('K',@name)"/>
    <xsl:text>&#x0A;{&#x0A;</xsl:text>
    <xsl:for-each select="const">
      <xsl:text>    </xsl:text>
      <xsl:value-of select="concat('K',../@name,'_',@name)"/>
      <xsl:text> = ::</xsl:text>
      <xsl:value-of select="concat(../@name,'_',@name)"/>
      <xsl:text>,&#x0A;</xsl:text>
    </xsl:for-each>
    <xsl:text>};&#x0A;&#x0A;Q_DECLARE_METATYPE(</xsl:text>
    <xsl:value-of select="concat('K',@name)"/>
    <xsl:text>)&#x0A;&#x0A;</xsl:text>
  </xsl:for-each>
  <xsl:text>&#x0A;&#x0A;</xsl:text>

  <xsl:apply-templates/>

</xsl:template>


<!--
 *  encloses |if| element's contents (unconditionally expanded by
 *  <apply-templates mode="define"/>) with #ifdef / #endif.
 *
 *  @note this can produce an empty #if/#endif block if |if|'s children
 *  expand to nothing (such as |cpp|). I see no need to handle this situation
 *  specially.
-->
<xsl:template match="if" mode="define">
  <xsl:if test="(@target='xpidl') or (@target='midl')">
    <xsl:apply-templates select="." mode="begin"/>
    <xsl:apply-templates mode="define"/>
    <xsl:apply-templates select="." mode="end"/>
    <xsl:text>&#x0A;</xsl:text>
  </xsl:if>
</xsl:template>


<!--
 *  encloses |if| element's contents (unconditionally expanded by
 *  <apply-templates mode="declare"/>) with #ifdef / #endif.
 *
 *  @note this can produce an empty #if/#endif block if |if|'s children
 *  expand to nothing (such as |cpp|). I see no need to handle this situation
 *  specially.
-->
<xsl:template match="if" mode="declare">
  <xsl:if test="(@target='xpidl') or (@target='midl')">
    <xsl:apply-templates select="." mode="begin"/>
    <xsl:apply-templates mode="declare"/>
    <xsl:apply-templates select="." mode="end"/>
    <xsl:text>&#x0A;</xsl:text>
  </xsl:if>
</xsl:template>


<!--
 *  |<if target="...">| element): begin and end.
-->
<xsl:template match="if" mode="begin">
  <xsl:if test="@target='xpidl'">
    <xsl:text>#if !defined (Q_WS_WIN32)&#x0A;</xsl:text>
  </xsl:if>
  <xsl:if test="@target='midl'">
    <xsl:text>#if defined (Q_WS_WIN32)&#x0A;</xsl:text>
  </xsl:if>
</xsl:template>
<xsl:template match="if" mode="end">
  <xsl:if test="(@target='xpidl') or (@target='midl')">
    <xsl:text>#endif&#x0A;</xsl:text>
  </xsl:if>
</xsl:template>


<!--
 *  cpp_quote
-->
<xsl:template match="cpp"/>


<!--
 *  #ifdef statement (@if attribute): begin and end
-->
<xsl:template match="@if" mode="begin">
  <xsl:text>#if </xsl:text>
  <xsl:value-of select="."/>
  <xsl:text>&#x0A;</xsl:text>
</xsl:template>
<xsl:template match="@if" mode="end">
  <xsl:text>#endif&#x0A;</xsl:text>
</xsl:template>


<!--
 *  libraries
-->
<xsl:template match="library">
  <!-- forward declarations -->
  <xsl:text>// forward declarations&#x0A;&#x0A;</xsl:text>
  <xsl:for-each select="interface">
    <xsl:text>class C</xsl:text>
    <xsl:value-of select="substring(@name,2)"/>
    <xsl:text>;&#x0A;</xsl:text>
  </xsl:for-each>
  <xsl:text>&#x0A;</xsl:text>
  <!-- array typedefs -->
  <xsl:text>// array typedefs&#x0A;&#x0A;</xsl:text>
  <xsl:for-each select="interface[not(@internal='yes')]">
    <xsl:if test="
      (//attribute[@safearray='yes' and not(@internal='yes') and @type=current()/@name])
      or
      (//param[@safearray='yes' and not(../@internal='yes') and @type=current()/@name])
    ">
      <xsl:text>typedef QVector &lt;C</xsl:text>
      <xsl:value-of select="substring(@name,2)"/>
      <xsl:text>&gt; C</xsl:text>
      <xsl:value-of select="substring(@name,2)"/>
      <xsl:text>Vector;&#x0A;</xsl:text>
    </xsl:if>
  </xsl:for-each>
  <xsl:text>&#x0A;</xsl:text>
  <!-- wrapper declarations -->
  <xsl:text>// wrapper declarations&#x0A;&#x0A;</xsl:text>
  <xsl:apply-templates select="
      if |
      interface[not(@internal='yes')]
    "
    mode="declare"
  />
  <!-- wrapper definitions -->
  <xsl:text>// wrapper definitions&#x0A;&#x0A;</xsl:text>
  <xsl:apply-templates select="
      if |
      interface[not(@internal='yes')]
    "
    mode="define"
  />
</xsl:template>


<!--
 *  interface declarations
-->
<xsl:template match="interface" mode="declare">

  <xsl:text>// </xsl:text>
  <xsl:value-of select="@name"/>
  <xsl:text> wrapper&#x0A;&#x0A;class C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text> : public CInterface &lt;</xsl:text>
  <xsl:value-of select="@name"/>
  <!-- use the correct base if supportsErrorInfo -->
  <xsl:call-template name="tryComposeFetchErrorInfo">
    <xsl:with-param name="mode" select="'getBaseClassName'"/>
  </xsl:call-template>
  <xsl:text>&gt;&#x0A;{&#x0A;public:&#x0A;&#x0A;</xsl:text>

  <!-- generate the Base typedef-->
  <xsl:text>    typedef CInterface &lt;</xsl:text>
  <xsl:value-of select="@name"/>
  <!-- Use the correct base if supportsErrorInfo -->
  <xsl:call-template name="tryComposeFetchErrorInfo">
    <xsl:with-param name="mode" select="'getBaseClassName'"/>
  </xsl:call-template>
  <xsl:text>&gt; Base;&#x0A;&#x0A;</xsl:text>

  <xsl:if test="name()='interface'">
    <xsl:call-template name="declareMembers"/>
  </xsl:if>

  <xsl:text>};&#x0A;&#x0A;</xsl:text>

</xsl:template>

<xsl:template name="declareAttributes">

  <xsl:param name="iface"/>

  <xsl:apply-templates select="$iface//attribute[not(@internal='yes')]" mode="declare"/>
  <xsl:if test="$iface//attribute[not(@internal='yes')]">
    <xsl:text>&#x0A;</xsl:text>
  </xsl:if>
  <!-- go to the base interface -->
  <xsl:if test="$iface/@extends and $iface/@extends!='$unknown'">
    <xsl:choose>
      <!-- interfaces within library/if -->
      <xsl:when test="name(..)='if'">
        <xsl:call-template name="declareAttributes">
          <xsl:with-param name="iface" select="
            preceding-sibling::
              *[self::interface and @name=$iface/@extends] |
            following-sibling::
              *[self::interface and @name=$iface/@extends] |
            ../preceding-sibling::if[@target=../@target]/
              *[self::interface and @name=$iface/@extends] |
            ../following-sibling::if[@target=../@target]/
              *[self::interface and @name=$iface/@extends]
          "/>
        </xsl:call-template>
      </xsl:when>
      <!-- interfaces within library -->
      <xsl:otherwise>
        <xsl:call-template name="declareAttributes">
          <xsl:with-param name="iface" select="
            preceding-sibling::
              *[self::interface and @name=$iface/@extends] |
            following-sibling::
              *[self::interface and @name=$iface/@extends]
          "/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:if>

</xsl:template>

<xsl:template name="declareMethods">

  <xsl:param name="iface"/>

  <xsl:apply-templates select="$iface//method[not(@internal='yes')]" mode="declare"/>
  <xsl:if test="$iface//method[not(@internal='yes')]">
    <xsl:text>&#x0A;</xsl:text>
  </xsl:if>
  <!-- go to the base interface -->
  <xsl:if test="$iface/@extends and $iface/@extends!='$unknown'">
    <xsl:choose>
      <!-- interfaces within library/if -->
      <xsl:when test="name(..)='if'">
        <xsl:call-template name="declareMethods">
          <xsl:with-param name="iface" select="
            preceding-sibling::
              *[self::interface and @name=$iface/@extends] |
            following-sibling::
              *[self::interface and @name=$iface/@extends] |
            ../preceding-sibling::if[@target=../@target]/
              *[self::interface and @name=$iface/@extends] |
            ../following-sibling::if[@target=../@target]/
              *[self::interface and @name=$iface/@extends]
          "/>
        </xsl:call-template>
      </xsl:when>
      <!-- interfaces within library -->
      <xsl:otherwise>
        <xsl:call-template name="declareMethods">
          <xsl:with-param name="iface" select="
            preceding-sibling::
              *[self::interface and @name=$iface/@extends] |
            following-sibling::
              *[self::interface and @name=$iface/@extends]
          "/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:if>

</xsl:template>

<xsl:template name="declareMembers">

  <xsl:text>    // constructors and assignments taking CUnknown and </xsl:text>
  <xsl:text>raw iface pointer&#x0A;&#x0A;</xsl:text>
  <!-- default constructor -->
  <xsl:text>    C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text> () {}&#x0A;&#x0A;</xsl:text>
  <!-- constructor taking CWhatever -->
  <xsl:text>    template &lt;class OI, class OB&gt; explicit C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
<xsl:text> (const CInterface &lt;OI, OB&gt; &amp; that)
    {
        attach (that.raw());
        if (SUCCEEDED (mRC))
        {
            mRC = that.lastRC();
            setErrorInfo (that.errorInfo());
        }
    }
</xsl:text>
  <xsl:text>&#x0A;</xsl:text>
  <!-- specialization for ourselves (copy constructor) -->
  <xsl:text>    C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text> (const C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text> &amp; that) : Base (that) {}&#x0A;&#x0A;</xsl:text>
  <!-- constructor taking a raw iface pointer -->
  <xsl:text>    template &lt;class OI&gt; explicit C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text> (OI * aIface) { attach (aIface); }&#x0A;&#x0A;</xsl:text>
  <!-- specialization for ourselves -->
  <xsl:text>    explicit C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text> (</xsl:text>
  <xsl:value-of select="@name"/>
  <xsl:text> * aIface) : Base (aIface) {}&#x0A;&#x0A;</xsl:text>
  <!-- assignment taking CWhatever -->
  <xsl:text>    template &lt;class OI, class OB&gt; C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
<xsl:text> &amp; operator = (const CInterface &lt;OI, OB&gt; &amp; that)
    {
        attach (that.raw());
        if (SUCCEEDED (mRC))
        {
            mRC = that.lastRC();
            setErrorInfo (that.errorInfo());
        }
        return *this;
    }
</xsl:text>
  <xsl:text>&#x0A;</xsl:text>
  <!-- specialization for ourselves -->
  <xsl:text>    C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text> &amp; operator = (const C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
<xsl:text> &amp; that)
    {
        Base::operator= (that);
        return *this;
    }
</xsl:text>
  <xsl:text>&#x0A;</xsl:text>
  <!-- assignment taking a raw iface pointer -->
  <xsl:text>    template &lt;class OI&gt; C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
<xsl:text> &amp; operator = (OI * aIface)
    {
        attach (aIface);
        return *this;
    }
</xsl:text>
  <xsl:text>&#x0A;</xsl:text>
  <!-- specialization for ourselves -->
  <xsl:text>    C</xsl:text>
  <xsl:value-of select="substring(@name,2)"/>
  <xsl:text> &amp; operator = (</xsl:text>
  <xsl:value-of select="@name"/>
<xsl:text> * aIface)
    {
        Base::operator= (aIface);
        return *this;
    }
</xsl:text>
  <xsl:text>&#x0A;</xsl:text>

  <xsl:text>    // attributes (properties)&#x0A;&#x0A;</xsl:text>
  <xsl:call-template name="declareAttributes">
    <xsl:with-param name="iface" select="."/>
  </xsl:call-template>

  <xsl:text>    // methods&#x0A;&#x0A;</xsl:text>
  <xsl:call-template name="declareMethods">
    <xsl:with-param name="iface" select="."/>
  </xsl:call-template>

  <xsl:text>    // friend wrappers&#x0A;&#x0A;</xsl:text>
  <xsl:text>    friend class CUnknown;&#x0A;</xsl:text>
  <xsl:variable name="name" select="@name"/>
  <xsl:variable name="parent" select=".."/>
  <!-- for definitions inside <if> -->
  <xsl:if test="name(..)='if'">
    <xsl:for-each select="
      preceding-sibling::*[self::interface] |
      following-sibling::*[self::interface] |
      ../preceding-sibling::*[self::interface] |
      ../following-sibling::*[self::interface] |
      ../preceding-sibling::if[@target=$parent/@target]/*[self::interface] |
      ../following-sibling::if[@target=$parent/@target]/*[self::interface]
    ">
      <xsl:if test="
        ((name()='interface')
         and
         ((name(..)!='if' and (if[@target=$parent/@target]/method/param[@type=$name]
                               or
                               if[@target=$parent/@target]/attribute[@type=$name]))
          or
          (.//method/param[@type=$name] or attribute[@type=$name])))
      ">
        <xsl:text>    friend class C</xsl:text>
        <xsl:value-of select="substring(@name,2)"/>
        <xsl:text>;&#x0A;</xsl:text>
      </xsl:if>
    </xsl:for-each>
  </xsl:if>
  <!-- for definitions outside <if> (i.e. inside <library>) -->
  <xsl:if test="name(..)!='if'">
    <xsl:for-each select="
      preceding-sibling::*[self::interface] |
      following-sibling::*[self::interface] |
      preceding-sibling::if/*[self::interface] |
      following-sibling::if/*[self::interface]
    ">
      <xsl:if test="
        name()='interface' and (.//method/param[@type=$name] or attribute[@type=$name])
      ">
        <xsl:text>    friend class C</xsl:text>
        <xsl:value-of select="substring(@name,2)"/>
        <xsl:text>;&#x0A;</xsl:text>
      </xsl:if>
    </xsl:for-each>
  </xsl:if>

</xsl:template>

<!-- attribute declarations -->
<xsl:template match="interface//attribute" mode="declare">
  <xsl:apply-templates select="parent::node()" mode="begin"/>
  <xsl:apply-templates select="@if" mode="begin"/>
  <xsl:call-template name="composeMethod">
    <xsl:with-param name="return" select="."/>
  </xsl:call-template>
  <xsl:if test="not(@readonly='yes')">
    <xsl:call-template name="composeMethod">
      <xsl:with-param name="return" select="''"/>
    </xsl:call-template>
  </xsl:if>
  <xsl:apply-templates select="@if" mode="end"/>
  <xsl:apply-templates select="parent::node()" mode="end"/>
</xsl:template>

<!-- method declarations -->
<xsl:template match="interface//method" mode="declare">
  <xsl:apply-templates select="parent::node()" mode="begin"/>
  <xsl:apply-templates select="@if" mode="begin"/>
  <xsl:call-template name="composeMethod"/>
  <xsl:apply-templates select="@if" mode="end"/>
  <xsl:apply-templates select="parent::node()" mode="end"/>
</xsl:template>


<!--
 *  interface definitions
-->
<xsl:template match="interface" mode="define">

  <xsl:text>// </xsl:text>
  <xsl:value-of select="@name"/>
  <xsl:text> wrapper&#x0A;&#x0A;</xsl:text>

  <xsl:if test="name()='interface'">
    <xsl:call-template name="defineMembers"/>
  </xsl:if>

</xsl:template>

<xsl:template name="defineAttributes">

  <xsl:param name="iface"/>

  <xsl:apply-templates select="$iface//attribute[not(@internal='yes')]" mode="define">
    <xsl:with-param name="namespace" select="."/>
  </xsl:apply-templates>

  <!-- go to the base interface -->
  <xsl:if test="$iface/@extends and $iface/@extends!='$unknown'">
    <xsl:choose>
      <!-- interfaces within library/if -->
      <xsl:when test="name(..)='if'">
        <xsl:call-template name="defineAttributes">
          <xsl:with-param name="iface" select="
            preceding-sibling::
              *[self::interface and @name=$iface/@extends] |
            following-sibling::
              *[self::interface and @name=$iface/@extends] |
            ../preceding-sibling::if[@target=../@target]/
              *[self::interface and @name=$iface/@extends] |
            ../following-sibling::if[@target=../@target]/
              *[self::interface and @name=$iface/@extends]
          "/>
        </xsl:call-template>
      </xsl:when>
      <!-- interfaces within library -->
      <xsl:otherwise>
        <xsl:call-template name="defineAttributes">
          <xsl:with-param name="iface" select="
            preceding-sibling::
              *[self::interface and @name=$iface/@extends] |
            following-sibling::
              *[self::interface and @name=$iface/@extends]
          "/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:if>

</xsl:template>

<xsl:template name="defineMethods">

  <xsl:param name="iface"/>

  <xsl:apply-templates select="$iface//method[not(@internal='yes')]" mode="define">
    <xsl:with-param name="namespace" select="."/>
  </xsl:apply-templates>

  <!-- go to the base interface -->
  <xsl:if test="$iface/@extends and $iface/@extends!='$unknown'">
    <xsl:choose>
      <!-- interfaces within library/if -->
      <xsl:when test="name(..)='if'">
        <xsl:call-template name="defineMethods">
          <xsl:with-param name="iface" select="
            preceding-sibling::
              *[self::interface and @name=$iface/@extends] |
            following-sibling::
              *[self::interface and @name=$iface/@extends] |
            ../preceding-sibling::if[@target=../@target]/
              *[self::interface and @name=$iface/@extends] |
            ../following-sibling::if[@target=../@target]/
              *[self::interface and @name=$iface/@extends]
          "/>
        </xsl:call-template>
      </xsl:when>
      <!-- interfaces within library -->
      <xsl:otherwise>
        <xsl:call-template name="defineMethods">
          <xsl:with-param name="iface" select="
            preceding-sibling::
              *[self::interface and @name=$iface/@extends] |
            following-sibling::
              *[self::interface and @name=$iface/@extends]
          "/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:if>

</xsl:template>

<xsl:template name="defineMembers">
  <xsl:call-template name="defineAttributes">
    <xsl:with-param name="iface" select="."/>
  </xsl:call-template>
  <xsl:call-template name="defineMethods">
    <xsl:with-param name="iface" select="."/>
  </xsl:call-template>
</xsl:template>

<!-- attribute definitions -->
<xsl:template match="interface//attribute" mode="define">

  <xsl:param name="namespace" select="ancestor::interface[1]"/>

  <xsl:apply-templates select="parent::node()" mode="begin"/>
  <xsl:apply-templates select="@if" mode="begin"/>
  <xsl:call-template name="composeMethod">
    <xsl:with-param name="return" select="."/>
    <xsl:with-param name="define" select="'yes'"/>
    <xsl:with-param name="namespace" select="$namespace"/>
  </xsl:call-template>
  <xsl:if test="not(@readonly='yes')">
    <xsl:call-template name="composeMethod">
      <xsl:with-param name="return" select="''"/>
      <xsl:with-param name="define" select="'yes'"/>
      <xsl:with-param name="namespace" select="$namespace"/>
    </xsl:call-template>
  </xsl:if>
  <xsl:apply-templates select="@if" mode="end"/>
  <xsl:apply-templates select="parent::node()" mode="end"/>
  <xsl:text>&#x0A;</xsl:text>

</xsl:template>

<!-- method definitions -->
<xsl:template match="interface//method" mode="define">

  <xsl:param name="namespace" select="ancestor::interface[1]"/>

  <xsl:apply-templates select="parent::node()" mode="begin"/>
  <xsl:apply-templates select="@if" mode="begin"/>
  <xsl:call-template name="composeMethod">
    <xsl:with-param name="define" select="'yes'"/>
    <xsl:with-param name="namespace" select="$namespace"/>
  </xsl:call-template>
  <xsl:apply-templates select="@if" mode="end"/>
  <xsl:apply-templates select="parent::node()" mode="end"/>
  <xsl:text>&#x0A;</xsl:text>

</xsl:template>


<!--
 *  co-classes
-->
<xsl:template match="module/class"/>


<!--
 *  enums
-->
<xsl:template match="enum"/>


<!--
 *  base template to produce interface methods
 *
 *  @param return
 *      - in <attribute> context, must be '.' for getters and
 *        '' for setters
 *      - in <method> context, must not be specified (the default value
 *        will apply)
 *  @param define
 *      'yes' to procuce inlined definition outside the class
 *      declaration, or
 *      empty string to produce method declaration only (w/o body)
 *  @param namespace
 *      actual interface node for which this method is being defined
 *      (necessary to properly set a class name for inherited methods).
 *      If not specified, will default to the parent interface
 *      node of the method being defined.
-->
<xsl:template name="composeMethod">
  <xsl:param name="return" select="param[@dir='return']"/>
  <xsl:param name="define" select="''"/>
  <xsl:param name="namespace" select="ancestor::interface[1]"/>
  <xsl:choose>
    <!-- no return value -->
    <xsl:when test="not($return)">
      <xsl:choose>
        <xsl:when test="$define">
          <xsl:text>inline </xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>    </xsl:text>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:text>void </xsl:text>
      <xsl:if test="$define">
        <xsl:text>C</xsl:text>
        <xsl:value-of select="substring($namespace/@name,2)"/>
        <xsl:text>::</xsl:text>
      </xsl:if>
      <xsl:call-template name="composeMethodDecl">
        <xsl:with-param name="isSetter" select="'yes'"/>
      </xsl:call-template>
      <xsl:if test="$define">
        <xsl:text>&#x0A;{&#x0A;</xsl:text>
        <!-- iface assertion -->
        <xsl:text>    Assert (mIface);&#x0A;</xsl:text>
        <xsl:text>    if (!mIface)&#x0A;        return;&#x0A;</xsl:text>
        <!-- method call -->
        <xsl:call-template name="composeMethodCall">
          <xsl:with-param name="isSetter" select="'yes'"/>
        </xsl:call-template>
        <xsl:text>}&#x0A;</xsl:text>
      </xsl:if>
      <xsl:if test="not($define)">
        <xsl:text>;&#x0A;</xsl:text>
      </xsl:if>
    </xsl:when>
    <!-- has a return value -->
    <xsl:when test="count($return) = 1">
      <xsl:choose>
        <xsl:when test="$define">
          <xsl:text>inline </xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>    </xsl:text>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:apply-templates select="$return/@type"/>
      <xsl:text> </xsl:text>
      <xsl:if test="$define">
        <xsl:text>C</xsl:text>
        <xsl:value-of select="substring($namespace/@name,2)"/>
        <xsl:text>::</xsl:text>
      </xsl:if>
      <xsl:call-template name="composeMethodDecl"/>
      <xsl:if test="$define">
        <xsl:text>&#x0A;{&#x0A;    </xsl:text>
        <xsl:apply-templates select="$return/@type"/>
        <xsl:text> a</xsl:text>
        <xsl:call-template name="capitalize">
          <xsl:with-param name="str" select="$return/@name"/>
        </xsl:call-template>
        <xsl:apply-templates select="$return/@type" mode="initializer"/>
        <xsl:text>;&#x0A;</xsl:text>
        <!-- iface assertion -->
        <xsl:text>    Assert (mIface);&#x0A;</xsl:text>
        <xsl:text>    if (!mIface)&#x0A;        return a</xsl:text>
        <xsl:call-template name="capitalize">
          <xsl:with-param name="str" select="$return/@name"/>
        </xsl:call-template>
        <xsl:text>;&#x0A;</xsl:text>
        <!-- method call -->
        <xsl:call-template name="composeMethodCall"/>
        <!-- return statement -->
        <xsl:text>    return a</xsl:text>
        <xsl:call-template name="capitalize">
          <xsl:with-param name="str" select="$return/@name"/>
        </xsl:call-template>
        <xsl:text>;&#x0A;}&#x0A;</xsl:text>
      </xsl:if>
      <xsl:if test="not($define)">
        <xsl:text>;&#x0A;</xsl:text>
      </xsl:if>
    </xsl:when>
    <!-- otherwise error -->
    <xsl:otherwise>
      <xsl:message terminate="yes">
        <xsl:text>More than one return value in method: </xsl:text>
        <xsl:value-of select="$namespace/@name"/>
        <xsl:text>::</xsl:text>
        <xsl:value-of select="@name"/>
      </xsl:message>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="composeMethodDecl">
  <xsl:param name="isSetter" select="''"/>
  <xsl:choose>
    <!-- attribute method call -->
    <xsl:when test="name()='attribute'">
      <xsl:choose>
        <xsl:when test="$isSetter">
          <!-- name -->
          <xsl:text>Set</xsl:text>
          <xsl:call-template name="capitalize">
            <xsl:with-param name="str" select="@name"/>
          </xsl:call-template>
          <xsl:text> (</xsl:text>
          <!-- parameter -->
          <xsl:apply-templates select="@type" mode="param"/>
          <xsl:text> a</xsl:text>
          <xsl:call-template name="capitalize">
            <xsl:with-param name="str" select="@name"/>
          </xsl:call-template>
          <xsl:text>)</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <!-- name -->
          <xsl:text>Get</xsl:text>
          <xsl:call-template name="capitalize">
            <xsl:with-param name="str" select="@name"/>
          </xsl:call-template>
          <xsl:text> (</xsl:text>
          <!-- const method -->
          <xsl:text>) const</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <!-- regular method call -->
    <xsl:when test="name()='method'">
      <!-- name -->
      <xsl:call-template name="capitalize">
        <xsl:with-param name="str" select="@name"/>
      </xsl:call-template>
      <xsl:text> (</xsl:text>
      <!-- parameters -->
      <xsl:for-each select="param[@dir!='return']">
        <xsl:apply-templates select="@type" mode="param"/>
        <xsl:text> a</xsl:text>
        <xsl:call-template name="capitalize">
          <xsl:with-param name="str" select="@name"/>
        </xsl:call-template>
        <xsl:if test="position() != last()">
          <xsl:text>, </xsl:text>
        </xsl:if>
      </xsl:for-each>
      <xsl:text>)</xsl:text>
      <!-- const method -->
      <xsl:if test="@const='yes'"> const</xsl:if>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<xsl:template name="composeMethodCall">
  <xsl:param name="isSetter" select="''"/>
  <!-- apply 'pre-call' hooks -->
  <xsl:choose>
    <xsl:when test="name()='attribute'">
      <xsl:call-template name="hooks">
        <xsl:with-param name="when" select="'pre-call'"/>
        <xsl:with-param name="isSetter" select="$isSetter"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="name()='method'">
      <xsl:for-each select="param">
        <xsl:call-template name="hooks">
          <xsl:with-param name="when" select="'pre-call'"/>
        </xsl:call-template>
      </xsl:for-each>
    </xsl:when>
  </xsl:choose>
  <!-- start the call -->
  <xsl:text>    mRC = mIface-></xsl:text>
  <xsl:choose>
    <!-- attribute method call -->
    <xsl:when test="name()='attribute'">
      <!-- method name -->
      <xsl:choose>
        <xsl:when test="$isSetter">
          <xsl:text>COMSETTER(</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>COMGETTER(</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:call-template name="capitalize">
        <xsl:with-param name="str" select="@name"/>
      </xsl:call-template>
      <xsl:text>) (</xsl:text>
      <!-- parameter -->
      <xsl:call-template name="composeMethodCallParam">
        <xsl:with-param name="isIn" select="$isSetter"/>
        <xsl:with-param name="isOut" select="not($isSetter)"/>
      </xsl:call-template>
    </xsl:when>
    <!-- regular method call -->
    <xsl:when test="name()='method'">
      <!-- method name -->
      <xsl:call-template name="capitalize">
        <xsl:with-param name="str" select="@name"/>
      </xsl:call-template>
      <xsl:text> (</xsl:text>
      <!-- parameters -->
      <xsl:for-each select="param">
        <xsl:call-template name="composeMethodCallParam"/>
        <xsl:if test="position() != last()">
          <xsl:text>, </xsl:text>
        </xsl:if>
      </xsl:for-each>
    </xsl:when>
  </xsl:choose>
  <xsl:text>);&#x0A;</xsl:text>
  <!-- apply 'post-call' hooks -->
  <xsl:choose>
    <xsl:when test="name()='attribute'">
      <xsl:call-template name="hooks">
        <xsl:with-param name="when" select="'post-call'"/>
        <xsl:with-param name="isSetter" select="$isSetter"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="name()='method'">
      <xsl:for-each select="param">
        <xsl:call-template name="hooks">
          <xsl:with-param name="when" select="'post-call'"/>
        </xsl:call-template>
      </xsl:for-each>
    </xsl:when>
  </xsl:choose>
  <!-- -->
  <xsl:call-template name="tryComposeFetchErrorInfo"/>
</xsl:template>

<!--
 *  Composes a 'fetch error info' call or returns the name of the
 *  appropriate base class name that provides error info functionality
 *  (depending on the mode parameter). Does nothing if the current
 *  interface does not support error info.
 *
 *  @param mode
 *      - 'getBaseClassName': expands to the base class name
 *      - any other value: composes a 'fetch error info' method call
-->
<xsl:template name="tryComposeFetchErrorInfo">
  <xsl:param name="mode" select="''"/>
  <xsl:variable name="ifaceSupportsErrorInfo" select="
    ancestor-or-self::interface[1]/@supportsErrorInfo
  "/>
  <xsl:variable name="librarySupportsErrorInfo" select="ancestor::library/@supportsErrorInfo"/>
  <xsl:choose>
    <xsl:when test="$ifaceSupportsErrorInfo">
      <xsl:call-template name="composeFetchErrorInfo">
        <xsl:with-param name="supports" select="string($ifaceSupportsErrorInfo)"/>
        <xsl:with-param name="mode" select="$mode"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="$librarySupportsErrorInfo">
      <xsl:call-template name="composeFetchErrorInfo">
        <xsl:with-param name="supports" select="string($librarySupportsErrorInfo)"/>
        <xsl:with-param name="mode" select="$mode"/>
      </xsl:call-template>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<xsl:template name="composeFetchErrorInfo">
  <xsl:param name="supports" select="''"/>
  <xsl:param name="mode" select="''"/>
  <xsl:choose>
    <xsl:when test="$mode='getBaseClassName'">
      <xsl:if test="$supports='strict' or $supports='yes'">
        <xsl:text>, COMBaseWithEI</xsl:text>
      </xsl:if>
    </xsl:when>
    <xsl:otherwise>
      <xsl:if test="$supports='strict' or $supports='yes'">
        <xsl:text>    if (mRC != S_OK)&#x0A;    {&#x0A;</xsl:text>
        <xsl:text>        fetchErrorInfo (mIface, &amp;COM_IIDOF (Base::Iface));&#x0A;</xsl:text>
        <xsl:if test="$supports='strict'">
          <xsl:text>        AssertMsg (errInfo.isFullAvailable(), </xsl:text>
          <xsl:text>("for RC=0x%08X\n", mRC));&#x0A;</xsl:text>
        </xsl:if>
        <xsl:text>    }&#x0A;</xsl:text>
      </xsl:if>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="composeMethodCallParam">

  <xsl:param name="isIn" select="@dir='in'"/>
  <xsl:param name="isOut" select="@dir='out' or @dir='return'"/>

  <xsl:variable name="self_target" select="current()/ancestor::if/@target"/>

  <xsl:choose>
    <!-- safearrays -->
    <xsl:when test="@safearray='yes'">
      <xsl:choose>
        <xsl:when test="$isIn">
          <xsl:text>ComSafeArrayAsInParam (</xsl:text>
          <xsl:value-of select="@name"/>
          <xsl:text>)</xsl:text>
        </xsl:when>
        <xsl:when test="$isOut">
          <xsl:text>ComSafeArrayAsOutParam (</xsl:text>
          <xsl:value-of select="@name"/>
          <xsl:text>)</xsl:text>
        </xsl:when>
      </xsl:choose>
    </xsl:when>
    <!-- string types -->
    <xsl:when test="@type = 'wstring' or @type = 'uuid'">
      <xsl:choose>
        <xsl:when test="$isIn">
          <xsl:text>BSTRIn (a</xsl:text>
          <xsl:call-template name="capitalize">
            <xsl:with-param name="str" select="@name"/>
          </xsl:call-template>
          <xsl:text>)</xsl:text>
        </xsl:when>
        <xsl:when test="$isOut">
          <xsl:text>BSTROut (a</xsl:text>
          <xsl:call-template name="capitalize">
            <xsl:with-param name="str" select="@name"/>
          </xsl:call-template>
          <xsl:text>)</xsl:text>
        </xsl:when>
      </xsl:choose>
    </xsl:when>
    <!-- enum types -->
    <xsl:when test="
      (ancestor::library/enum[@name=current()/@type]) or
      (ancestor::library/if[@target=$self_target]/enum[@name=current()/@type])
    ">
      <xsl:choose>
        <xsl:when test="$isIn">
          <xsl:text>(</xsl:text>
          <xsl:value-of select="@type"/>
          <xsl:text>_T) a</xsl:text>
          <xsl:call-template name="capitalize">
            <xsl:with-param name="str" select="@name"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:when test="$isOut">
          <xsl:text>ENUMOut &lt;K</xsl:text>
          <xsl:value-of select="@type"/>
          <xsl:text>, </xsl:text>
          <xsl:value-of select="@type"/>
          <xsl:text>_T&gt; (a</xsl:text>
          <xsl:call-template name="capitalize">
            <xsl:with-param name="str" select="@name"/>
          </xsl:call-template>
          <xsl:text>)</xsl:text>
        </xsl:when>
      </xsl:choose>
    </xsl:when>
    <!-- interface types -->
    <xsl:when test="
      @type='$unknown' or
      ((ancestor::library/interface[@name=current()/@type]) or
       (ancestor::library/if[@target=$self_target]/interface[@name=current()/@type])
      )
    ">
      <xsl:choose>
        <xsl:when test="$isIn">
          <xsl:text>a</xsl:text>
          <xsl:call-template name="capitalize">
            <xsl:with-param name="str" select="@name"/>
          </xsl:call-template>
          <xsl:choose>
            <xsl:when test="@type='$unknown'">
              <xsl:text>.raw()</xsl:text>
            </xsl:when>
            <xsl:otherwise>
              <xsl:text>.mIface</xsl:text>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:when test="$isOut">
          <xsl:text>&amp;a</xsl:text>
          <xsl:call-template name="capitalize">
            <xsl:with-param name="str" select="@name"/>
          </xsl:call-template>
          <xsl:choose>
            <xsl:when test="@type='$unknown'">
              <xsl:text>.rawRef()</xsl:text>
            </xsl:when>
            <xsl:otherwise>
              <xsl:text>.mIface</xsl:text>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
      </xsl:choose>
    </xsl:when>
    <!-- currently unsupported types -->
    <xsl:when test="@type = 'string'">
      <xsl:message terminate="yes">
        <xsl:text>Parameter type </xsl:text>
        <xsl:value-of select="@type"/>
        <xsl:text>is not currently supported</xsl:text>
      </xsl:message>
    </xsl:when>
    <!-- assuming scalar types -->
    <xsl:otherwise>
      <xsl:choose>
        <xsl:when test="$isIn">
          <xsl:text>a</xsl:text>
          <xsl:call-template name="capitalize">
            <xsl:with-param name="str" select="@name"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:when test="$isOut">
          <xsl:text>&amp;a</xsl:text>
          <xsl:call-template name="capitalize">
            <xsl:with-param name="str" select="@name"/>
          </xsl:call-template>
        </xsl:when>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--
 *  attribute/parameter type conversion (returns plain Qt type name)
-->
<xsl:template match="attribute/@type | param/@type">
  <xsl:variable name="self_target" select="current()/ancestor::if/@target"/>

  <xsl:choose>
    <!-- modifiers -->
    <xsl:when test="name(current())='type' and ../@mod">
      <xsl:if test="../@safearray and ../@mod='ptr'">
        <xsl:message terminate="yes">
          <xsl:value-of select="concat(../../../@name,'::',../../@name,'::',../@name,': ')"/>
          <xsl:text>either 'safearray' or 'mod' attribute is allowed, but not both!</xsl:text>
        </xsl:message>
      </xsl:if>
      <xsl:choose>
        <xsl:when test="../@mod='ptr'">
          <xsl:choose>
            <!-- standard types -->
            <!--xsl:when test=".='result'">??</xsl:when-->
            <xsl:when test=".='boolean'">BOOL *</xsl:when>
            <xsl:when test=".='octet'">BYTE *</xsl:when>
            <xsl:when test=".='short'">SHORT *</xsl:when>
            <xsl:when test=".='unsigned short'">USHORT *</xsl:when>
            <xsl:when test=".='long'">LONG *</xsl:when>
            <xsl:when test=".='long long'">LONG64 *</xsl:when>
            <xsl:when test=".='unsigned long'">ULONG *</xsl:when>
            <xsl:when test=".='unsigned long long'">ULONG64 *</xsl:when>
            <xsl:otherwise>
              <xsl:message terminate="yes">
                <xsl:value-of select="concat(../../../@name,'::',../../@name,'::',../@name,': ')"/>
                <xsl:text>attribute 'mod=</xsl:text>
                <xsl:value-of select="concat('&quot;',../@mod,'&quot;')"/>
                <xsl:text>' cannot be used with type </xsl:text>
                <xsl:value-of select="concat('&quot;',current(),'&quot;!')"/>
              </xsl:message>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:when test="../@mod='string'">
          <xsl:if test="../@safearray">
            <xsl:text>QVector &lt;</xsl:text>
          </xsl:if>
          <xsl:choose>
            <!-- standard types -->
            <!--xsl:when test=".='result'">??</xsl:when-->
            <xsl:when test=".='uuid'">QString</xsl:when>
            <xsl:otherwise>
              <xsl:message terminate="yes">
                <xsl:value-of select="concat(../../../@name,'::',../../@name,'::',../@name,': ')"/>
                <xsl:text>attribute 'mod=</xsl:text>
                <xsl:value-of select="concat('&quot;',../@mod,'&quot;')"/>
                <xsl:text>' cannot be used with type </xsl:text>
                <xsl:value-of select="concat('&quot;',current(),'&quot;!')"/>
              </xsl:message>
            </xsl:otherwise>
          </xsl:choose>
          <xsl:if test="../@safearray">
            <xsl:text>&gt;</xsl:text>
          </xsl:if>
        </xsl:when>
        <xsl:otherwise>
          <xsl:message terminate="yes">
            <xsl:value-of select="concat(../../../@name,'::',../../@name,'::',../@name,': ')"/>
            <xsl:value-of select="concat('value &quot;',../@mod,'&quot; ')"/>
            <xsl:text>of attribute 'mod' is invalid!</xsl:text>
          </xsl:message>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <!-- no modifiers -->
    <xsl:otherwise>
      <xsl:if test="../@safearray">
        <xsl:text>QVector &lt;</xsl:text>
      </xsl:if>
      <xsl:choose>
        <!-- standard types -->
        <xsl:when test=".='result'">HRESULT</xsl:when>
        <xsl:when test=".='boolean'">BOOL</xsl:when>
        <xsl:when test=".='octet'">BYTE</xsl:when>
        <xsl:when test=".='short'">SHORT</xsl:when>
        <xsl:when test=".='unsigned short'">USHORT</xsl:when>
        <xsl:when test=".='long'">LONG</xsl:when>
        <xsl:when test=".='long long'">LONG64</xsl:when>
        <xsl:when test=".='unsigned long'">ULONG</xsl:when>
        <xsl:when test=".='unsigned long long'">ULONG64</xsl:when>
        <xsl:when test=".='char'">CHAR</xsl:when>
        <xsl:when test=".='string'">CHAR *</xsl:when>
        <xsl:when test=".='wchar'">OLECHAR</xsl:when>
        <xsl:when test=".='wstring'">QString</xsl:when>
        <!-- UUID type -->
        <xsl:when test=".='uuid'">QUuid</xsl:when>
        <!-- system interface types -->
        <xsl:when test=".='$unknown'">CUnknown</xsl:when>
        <xsl:otherwise>
          <xsl:choose>
            <!-- enum types -->
            <xsl:when test="
              (ancestor::library/enum[@name=current()]) or
              (ancestor::library/if[@target=$self_target]/enum[@name=current()])
            ">
              <xsl:value-of select="concat('K',string(.))"/>
            </xsl:when>
            <!-- custom interface types -->
            <xsl:when test="
              ((ancestor::library/interface[@name=current()]) or
               (ancestor::library/if[@target=$self_target]/interface[@name=current()])
              )
            ">
              <xsl:value-of select="concat('C',substring(.,2))"/>
            </xsl:when>
            <!-- other types -->
            <xsl:otherwise>
              <xsl:message terminate="yes">
                <xsl:text>Unknown parameter type: </xsl:text>
                <xsl:value-of select="."/>
              </xsl:message>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:if test="../@safearray">
        <xsl:text>&gt;</xsl:text>
      </xsl:if>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--
 *  generates a null initializer for all scalar types (such as bool or long)
 *  and enum types in the form of ' = <null_initializer>', or nothing for other
 *  types.
-->
<xsl:template match="attribute/@type | param/@type" mode="initializer">

  <xsl:variable name="self_target" select="current()/ancestor::if/@target"/>

  <xsl:choose>
    <!-- safearrays don't need initializers -->
    <xsl:when test="../@safearray">
    </xsl:when>
    <!-- modifiers -->
    <xsl:when test="name(current())='type' and ../@mod">
      <xsl:choose>
        <xsl:when test="../@mod='ptr'">
          <xsl:choose>
            <!-- standard types -->
            <!--xsl:when test=".='result'">??</xsl:when-->
            <xsl:when test=".='boolean'"> = NULL</xsl:when>
            <xsl:when test=".='octet'"> = NULL</xsl:when>
            <xsl:when test=".='short'"> = NULL</xsl:when>
            <xsl:when test=".='unsigned short'"> = NULL</xsl:when>
            <xsl:when test=".='long'"> = NULL</xsl:when>
            <xsl:when test=".='long long'"> = NULL</xsl:when>
            <xsl:when test=".='unsigned long'"> = NULL</xsl:when>
            <xsl:when test=".='unsigned long long'"> = NULL</xsl:when>
            <xsl:otherwise>
              <xsl:message terminate="yes">
                <xsl:value-of select="concat(../../../@name,'::',../../@name,'::',../@name,': ')"/>
                <xsl:text>attribute 'mod=</xsl:text>
                <xsl:value-of select="concat('&quot;',../@mod,'&quot;')"/>
                <xsl:text>' cannot be used with type </xsl:text>
                <xsl:value-of select="concat('&quot;',current(),'&quot;!')"/>
              </xsl:message>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:when test="../@mod='string'">
          <xsl:choose>
            <!-- standard types -->
            <!--xsl:when test=".='result'">??</xsl:when-->
            <xsl:when test=".='uuid'"></xsl:when>
            <xsl:otherwise>
              <xsl:message terminate="yes">
                <xsl:value-of select="concat(../../../@name,'::',../../@name,'::',../@name,': ')"/>
                <xsl:text>attribute 'mod=</xsl:text>
                <xsl:value-of select="concat('&quot;',../@mod,'&quot;')"/>
                <xsl:text>' cannot be used with type </xsl:text>
                <xsl:value-of select="concat('&quot;',current(),'&quot;!')"/>
              </xsl:message>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:otherwise>
          <xsl:message terminate="yes">
            <xsl:value-of select="concat(../../../@name,'::',../../@name,'::',../@name,': ')"/>
            <xsl:value-of select="concat('value &quot;',../@mod,'&quot; ')"/>
            <xsl:text>of attribute 'mod' is invalid!</xsl:text>
          </xsl:message>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <!-- no modifiers -->
    <xsl:otherwise>
      <xsl:choose>
        <!-- standard types that need a zero initializer -->
        <xsl:when test=".='result'"> = S_OK</xsl:when>
        <xsl:when test=".='boolean'"> = FALSE</xsl:when>
        <xsl:when test=".='octet'"> = 0</xsl:when>
        <xsl:when test=".='short'"> = 0</xsl:when>
        <xsl:when test=".='unsigned short'"> = 0</xsl:when>
        <xsl:when test=".='long'"> = 0</xsl:when>
        <xsl:when test=".='long long'"> = 0</xsl:when>
        <xsl:when test=".='unsigned long'"> = 0</xsl:when>
        <xsl:when test=".='unsigned long long'"> = 0</xsl:when>
        <xsl:when test=".='char'"> = 0</xsl:when>
        <xsl:when test=".='string'"> = NULL</xsl:when>
        <xsl:when test=".='wchar'"> = 0</xsl:when>
        <xsl:otherwise>
          <xsl:choose>
            <!-- enum types initialized with 0 -->
            <xsl:when test="
              (ancestor::library/enum[@name=current()]) or
              (ancestor::library/if[@target=$self_target]/enum[@name=current()])
            ">
              <xsl:value-of select="concat(' = (K',string(.),') 0')"/>
            </xsl:when>
          </xsl:choose>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--
 *  attribute/parameter type conversion (for method declaration)
-->
<xsl:template match="attribute/@type | param/@type" mode="param">

  <xsl:variable name="self_target" select="current()/ancestor::if/@target"/>

  <xsl:choose>
    <!-- class types -->
    <xsl:when test="
      .='string' or
      .='wstring' or
      ../@safearray='yes' or
      ((ancestor::library/enum[@name=current()]) or
       (ancestor::library/if[@target=$self_target]/enum[@name=current()])
      ) or
      .='$unknown' or
      ((ancestor::library/interface[@name=current()]) or
       (ancestor::library/if[@target=$self_target]/interface[@name=current()])
      )
    ">
      <xsl:choose>
        <!-- <attribute> context -->
        <xsl:when test="name(..)='attribute'">
          <xsl:text>const </xsl:text>
          <xsl:apply-templates select="."/>
          <xsl:text> &amp;</xsl:text>
        </xsl:when>
        <!-- <param> context -->
        <xsl:when test="name(..)='param'">
          <xsl:choose>
            <xsl:when test="../@dir='in'">
              <xsl:text>const </xsl:text>
              <xsl:apply-templates select="."/>
              <xsl:text> &amp;</xsl:text>
            </xsl:when>
            <xsl:when test="../@dir='out'">
              <xsl:apply-templates select="."/>
              <xsl:text> &amp;</xsl:text>
            </xsl:when>
            <xsl:when test="../@dir='return'">
              <xsl:apply-templates select="."/>
            </xsl:when>
          </xsl:choose>
        </xsl:when>
      </xsl:choose>
    </xsl:when>
    <!-- assume scalar types -->
    <xsl:otherwise>
      <xsl:choose>
        <!-- <attribute> context -->
        <xsl:when test="name(..)='attribute'">
          <xsl:apply-templates select="."/>
        </xsl:when>
        <!-- <param> context -->
        <xsl:when test="name(..)='param'">
          <xsl:apply-templates select="."/>
          <xsl:if test="../@dir='out'">
            <xsl:text> &amp;</xsl:text>
          </xsl:if>
        </xsl:when>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--
 *  attribute/parameter type conversion (returns plain COM type name)
 *  (basically, copied from midl.xsl)
-->
<xsl:template match="attribute/@type | param/@type" mode="com">

  <xsl:variable name="self_target" select="current()/ancestor::if/@target"/>

  <xsl:choose>
    <!-- modifiers -->
    <xsl:when test="name(current())='type' and ../@mod">
      <xsl:choose>
        <xsl:when test="../@mod='ptr'">
          <xsl:choose>
            <!-- standard types -->
            <!--xsl:when test=".='result'">??</xsl:when-->
            <xsl:when test=".='boolean'">BOOL *</xsl:when>
            <xsl:when test=".='octet'">BYTE *</xsl:when>
            <xsl:when test=".='short'">SHORT *</xsl:when>
            <xsl:when test=".='unsigned short'">USHORT *</xsl:when>
            <xsl:when test=".='long'">LONG *</xsl:when>
            <xsl:when test=".='long long'">LONG64 *</xsl:when>
            <xsl:when test=".='unsigned long'">ULONG *</xsl:when>
            <xsl:when test=".='unsigned long long'">ULONG64 *</xsl:when>
            <xsl:otherwise>
              <xsl:message terminate="yes">
                <xsl:value-of select="concat(../../../@name,'::',../../@name,'::',../@name,': ')"/>
                <xsl:text>attribute 'mod=</xsl:text>
                <xsl:value-of select="concat('&quot;',../@mod,'&quot;')"/>
                <xsl:text>' cannot be used with type </xsl:text>
                <xsl:value-of select="concat('&quot;',current(),'&quot;!')"/>
              </xsl:message>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:when test="../@mod='string'">
          <xsl:choose>
            <!-- standard types -->
            <!--xsl:when test=".='result'">??</xsl:when-->
            <xsl:when test=".='uuid'">BSTR</xsl:when>
            <xsl:otherwise>
              <xsl:message terminate="yes">
                <xsl:value-of select="concat(../../../@name,'::',../../@name,'::',../@name,': ')"/>
                <xsl:text>attribute 'mod=</xsl:text>
                <xsl:value-of select="concat('&quot;',../@mod,'&quot;')"/>
                <xsl:text>' cannot be used with type </xsl:text>
                <xsl:value-of select="concat('&quot;',current(),'&quot;!')"/>
              </xsl:message>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:otherwise>
          <xsl:message terminate="yes">
            <xsl:value-of select="concat(../../../@name,'::',../../@name,'::',../@name,': ')"/>
            <xsl:value-of select="concat('value &quot;',../@mod,'&quot; ')"/>
            <xsl:text>of attribute 'mod' is invalid!</xsl:text>
          </xsl:message>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <!-- no modifiers -->
    <xsl:otherwise>
      <xsl:choose>
        <!-- standard types -->
        <xsl:when test=".='result'">HRESULT</xsl:when>
        <xsl:when test=".='boolean'">BOOL</xsl:when>
        <xsl:when test=".='octet'">BYTE</xsl:when>
        <xsl:when test=".='short'">SHORT</xsl:when>
        <xsl:when test=".='unsigned short'">USHORT</xsl:when>
        <xsl:when test=".='long'">LONG</xsl:when>
        <xsl:when test=".='long long'">LONG64</xsl:when>
        <xsl:when test=".='unsigned long'">ULONG</xsl:when>
        <xsl:when test=".='unsigned long long'">ULONG64</xsl:when>
        <xsl:when test=".='char'">CHAR</xsl:when>
        <xsl:when test=".='string'">CHAR *</xsl:when>
        <xsl:when test=".='wchar'">OLECHAR</xsl:when>
        <xsl:when test=".='wstring'">BSTR</xsl:when>
        <!-- UUID type -->
        <xsl:when test=".='uuid'">GUID</xsl:when>
        <!-- system interface types -->
        <xsl:when test=".='$unknown'">IUnknown *</xsl:when>
        <xsl:otherwise>
          <xsl:choose>
            <!-- enum types -->
            <xsl:when test="
              (ancestor::library/enum[@name=current()]) or
              (ancestor::library/if[@target=$self_target]/enum[@name=current()])
            ">
              <xsl:value-of select="."/>
            </xsl:when>
            <!-- custom interface types -->
            <xsl:when test="
              ((ancestor::library/interface[@name=current()]) or
               (ancestor::library/if[@target=$self_target]/interface[@name=current()])
              )
            ">
              <xsl:value-of select="."/><xsl:text> *</xsl:text>
            </xsl:when>
            <!-- other types -->
            <xsl:otherwise>
              <xsl:message terminate="yes">
                <xsl:text>Unknown parameter type: </xsl:text>
                <xsl:value-of select="."/>
              </xsl:message>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!--
 *  attribute/parameter type additional hooks.
 *
 *  Called in the context of <attribute> or <param> elements.
 *
 *  @param when     When the hook is being called:
 *                  'pre-call'  - right before the method call
 *                  'post-call' - right after the method call
 *  @param isSetter Non-empty if called in the cotext of the attribute setter
 *                  call.
-->
<xsl:template name="hooks">

  <xsl:param name="when" select="''"/>
  <xsl:param name="isSetter" select="''"/>

  <xsl:variable name="self_target" select="current()/ancestor::if/@target"/>

  <xsl:variable name="is_iface" select="(
    ((ancestor::library/interface[@name=current()/@type]) or
     (ancestor::library/if[@target=$self_target]/interface[@name=current()/@type])
    )
  )"/>

  <xsl:variable name="is_enum" select="(
    (ancestor::library/enum[@name=current()/@type]) or
    (ancestor::library/if[@target=$self_target]/enum[@name=current()/@type])
  )"/>

  <xsl:choose>
    <xsl:when test="$when='pre-call'">
      <xsl:choose>
        <xsl:when test="@safearray='yes'">
          <!-- declare a SafeArray variable -->
          <xsl:choose>
            <!-- interface types need special treatment here -->
            <xsl:when test="@type='$unknown'">
              <xsl:text>    com::SafeIfaceArray &lt;IUnknown&gt; </xsl:text>
            </xsl:when>
            <xsl:when test="$is_iface">
              <xsl:text>    com::SafeIfaceArray &lt;</xsl:text>
              <xsl:value-of select="@type"/>
              <xsl:text>&gt; </xsl:text>
            </xsl:when>
            <!-- enums need the _T prefix -->
            <xsl:when test="$is_enum">
              <xsl:text>    com::SafeArray &lt;</xsl:text>
              <xsl:value-of select="@type"/>
              <xsl:text>_T&gt; </xsl:text>
            </xsl:when>
            <!-- GUID is special too -->
            <xsl:when test="@type='uuid' and @mod!='string'">
              <xsl:text>    com::SafeGUIDArray </xsl:text>
            </xsl:when>
            <!-- everything else is not -->
            <xsl:otherwise>
              <xsl:text>    com::SafeArray &lt;</xsl:text>
              <xsl:apply-templates select="@type" mode="com"/>
              <xsl:text>&gt; </xsl:text>
            </xsl:otherwise>
          </xsl:choose>
          <xsl:value-of select="@name"/>
          <xsl:text>;&#x0A;</xsl:text>
          <xsl:if test="(name()='attribute' and $isSetter) or
                        (name()='param' and @dir='in')">
            <!-- convert QVector to SafeArray -->
            <xsl:choose>
              <!-- interface types need special treatment here -->
              <xsl:when test="@type='$unknown' or $is_iface">
                <xsl:text>    ToSafeIfaceArray (</xsl:text>
              </xsl:when>
              <xsl:otherwise>
                <xsl:text>    ToSafeArray (</xsl:text>
              </xsl:otherwise>
            </xsl:choose>
            <xsl:text>a</xsl:text>
            <xsl:call-template name="capitalize">
              <xsl:with-param name="str" select="@name"/>
            </xsl:call-template>
            <xsl:text>, </xsl:text>
            <xsl:value-of select="@name"/>
            <xsl:text>);&#x0A;</xsl:text>
          </xsl:if>
        </xsl:when>
      </xsl:choose>
    </xsl:when>
    <xsl:when test="$when='post-call'">
      <xsl:choose>
        <xsl:when test="@safearray='yes'">
          <xsl:if test="(name()='attribute' and not($isSetter)) or
                        (name()='param' and (@dir='out' or @dir='return'))">
            <!-- convert SafeArray to QVector -->
            <xsl:choose>
              <!-- interface types need special treatment here -->
              <xsl:when test="@type='$unknown' or $is_iface">
                <xsl:text>    FromSafeIfaceArray (</xsl:text>
              </xsl:when>
              <xsl:otherwise>
                <xsl:text>    FromSafeArray (</xsl:text>
              </xsl:otherwise>
            </xsl:choose>
            <xsl:value-of select="@name"/>
            <xsl:text>, </xsl:text>
            <xsl:text>a</xsl:text>
            <xsl:call-template name="capitalize">
              <xsl:with-param name="str" select="@name"/>
            </xsl:call-template>
            <xsl:text>);&#x0A;</xsl:text>
          </xsl:if>
        </xsl:when>
      </xsl:choose>
    </xsl:when>
    <xsl:otherwise>
      <xsl:message terminate="yes">
        <xsl:text>Invalid when value: </xsl:text>
        <xsl:value-of select="$when"/>
      </xsl:message>
    </xsl:otherwise>
  </xsl:choose>

</xsl:template>


</xsl:stylesheet>
