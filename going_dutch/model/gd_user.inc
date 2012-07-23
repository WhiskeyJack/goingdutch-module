<?php

class GD_User {

  var $DrupalUid;
  var $GDid;
  var $Details;
  protected $Groups;
  protected $GroupPages;

  function GD_User($userid, $useridtype='drupal') {
    if ($useridtype == 'drupal')
      $this->DrupalUid = $userid;
    else
      $this->GDid = $userid;
    $this->fetch_details();
  }

  /**
   * Retrieves the Going Dutch user details 
   * User details are stored in $this->Details as an object
   */
  function fetch_details() {
    if (!gd_connect_db())
      return FALSE;
    $query = db_select('users', 'u');
    $query->leftJoin('preferences', 'p', 'u.user_id = p.user_id');
    $query->fields('u', array('user_id', 'drupal_id', 'username', 'password', 'email', 'realname', 'activated', 'confirmation', 'reg_date', 'last_login'));
    $query->fields('p');
    if (isset($this->DrupalUid) && !empty($this->DrupalUid))
      $query->condition('drupal_id', $this->DrupalUid, '=');
    else
      $query->condition('u.user_id', $this->GDid, '=');
    $this->Details = $query->execute()->fetchObject();
    db_set_active();
    if (!$this->Details)
      return FALSE;
    if (!isset($this->Details->number_format))
      $this->Details->number_format = '{"thousand_sep":".","decimal_sep":",","decimal_count":2,"currency_symbol":"&#8364;","currency_location":"before"}';
    $this->Details->number_format = json_decode($this->Details->number_format);
    $this->DrupalUid = $this->Details->drupal_id;
    $this->GDid = $this->Details->user_id;
    if (!isset($this->Details->name_format) || $this->Details->name_format == 0)
      $this->Details->name_format = 2;
    switch ($this->Details->name_format) {
      case 1:
        $this->Details->name_format = array('realname', 'username');
        break;
      case 2:
        $this->Details->name_format = array('realname');
        break;
      case 3:
        $this->Details->name_format = array('username', 'realname');
        break;
      case 4:
        $this->Details->name_format = array('username');
        break;
    }
  }

  //  {"thousand_sep":".","decimal_sep":",","decimal_count":2}
  /**
   * Retrieves the Going Dutch user groups the user is a member of
   * Group details are stored in $this->Groups as an object
   */
  function fetch_groups() {
    gd_connect_db();
    $sql = "SELECT `groups`.*, `users_groups`.`role_id`, `users_groups`.`join_date` 
                  FROM `users_groups`, `groups`  WHERE `users_groups`.`group_id` = `groups`.`group_id`
                  AND removed = 0
                  AND `user_id` IN (SELECT `user_id` FROM `users` WHERE `user_id` = {$this->GDid})";
    $result = db_query($sql);
    $this->Groups = $result->fetchAllAssoc('group_id');
    db_set_active();
  }

  /**
   * Checks if the given group id is in the list of groups for this user
   * @param type $groupid
   * @return type boolean
   */
  function is_group_member($groupid) {
    if (key_exists($groupid, $this->get_groups()))
      return TRUE;
    return FALSE;
  }

  /**
   * Checks if the user may access the page specified for this group
   * Returns true if the role_id of the group is lower than the page_access
   * @param type $groupid
   * @return type boolean
   */
  function may_access_page($groupid, $gd_page) {
    if (empty($this->GroupPages))
      $this->GroupPages = gd_get_grouppages();

    if (key_exists($groupid, $this->get_groups()) &&
            ($this->Groups[$groupid]->role_id <= 
            $this->GroupPages[$gd_page]->page_access))
      return TRUE;

    return FALSE;
  }

  /**
   * Returns true if the user is an owner of the given groupid
   * @param type $groupid
   * @return type 
   */
  function is_group_owner($groupid) {
    if (empty($this->Groups))
      $this->fetch_groups();
    if ($this->Groups[$groupid]->role_id == 0)
      return TRUE;
    return FALSE;
  }

  /**
   * Returns the groups the user is a member of
   * @return object with keys group_id, name, description, 
   * reg_date (datetime), role_id and join_date (datetime)
   */
  function get_groups() {
    if (empty($this->Groups)) {
      $this->fetch_groups();
    }
    return $this->Groups;
  }

}